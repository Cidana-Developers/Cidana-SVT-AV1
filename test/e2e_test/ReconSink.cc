/*
* Copyright(c) 2019 Netflix, Inc.
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/******************************************************************************
 * @file ReconSink.cc
 *
 * @brief Impelmentation of reconstruction frame sink
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#include <stdio.h>
#include <vector>
#include <algorithm>
#include "ReconSink.h"

#if _WIN32
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#define FOPEN(f, s, m) fopen_s(&f, s, m)
#else
#define fseeko64 fseek
#define ftello64 ftell
#define FOPEN(f, s, m) f = fopen(s, m)
#endif

static void delete_mug(ReconSink::ReconMug *mug) {
    if (mug) {
        if (mug->mug_buf) {
            delete[] mug->mug_buf;
        }
        delete mug;
    }
}

class ReconSinkFile : public ReconSink {
  public:
    ReconSinkFile(const VideoFrameParam &param, const char *file_path)
        : ReconSink(param) {
        sink_type_ = RECON_SINK_FILE;
        max_frame_ts_ = 0;
        FOPEN(recon_file_, file_path, "wb");
        record_list_.clear();
    }
    virtual ~ReconSinkFile() {
        if (recon_file_) {
            fflush(recon_file_);
            fclose(recon_file_);
            recon_file_ = nullptr;
        }
        record_list_.clear();
    }
    void fill_mug(ReconMug *mug) override {
        if (recon_file_ && mug->filled_size &&
            mug->filled_size <= mug->mug_size &&
            mug->time_stamp < (uint64_t)frame_count_) {
            if (mug->time_stamp >=
                max_frame_ts_) {  // new frame is larger than max timestamp
                fseeko64(recon_file_, 0, SEEK_END);
                for (size_t i = max_frame_ts_; i < mug->time_stamp + 1; ++i) {
                    fwrite(mug->mug_buf, 1, mug->mug_size, recon_file_);
                }
                max_frame_ts_ = mug->time_stamp;
            }

            rewind(recon_file_);
            uint64_t frameNum = mug->time_stamp;
            while (frameNum > 0) {
                int ret = fseeko64(recon_file_, mug->filled_size, SEEK_CUR);
                if (ret != 0) {
                    return;
                }
                frameNum--;
            }
            fwrite(mug->mug_buf, 1, mug->filled_size, recon_file_);
            fflush(recon_file_);
            record_list_.push_back((uint32_t)mug->time_stamp);
        }
        delete_mug(mug);
    }
    const ReconMug *take_mug(const uint64_t time_stamp) override {
        if (recon_file_ == nullptr)
            return nullptr;

        ReconMug *mug = nullptr;
        fseeko64(recon_file_, 0, SEEK_END);
        int64_t actual_size = ftello64(recon_file_);
        if (actual_size > 0 &&
            ((uint64_t)actual_size) > ((time_stamp + 1) * frame_size_)) {
            int ret = fseeko64(recon_file_, time_stamp * frame_size_, 0);
            if (ret != 0) {
                // printf("Error in fseeko64  returnVal %i\n", ret);
                return nullptr;
            }
            mug = get_empty_mug();
            if (mug) {
                size_t read_size =
                    fread(mug->mug_buf, 1, frame_size_, recon_file_);
                if (read_size <= 0) {
                    pour_mug(mug);
                    mug = nullptr;
                }
                mug->filled_size = (uint32_t)read_size;
                mug->time_stamp = time_stamp;
            }
        }

        return mug;
    }
    const ReconMug *take_mug_inorder(const uint32_t index) override {
        return take_mug(index);
    }
    void pour_mug(ReconMug *mug) override {
        delete_mug(mug);
    }
    bool is_compelete() override {
        if (record_list_.size() < frame_count_)
            return false;
        std::sort(record_list_.begin(), record_list_.end());
        if (record_list_.at(frame_count_ - 1) != frame_count_ - 1)
            return false;
        return true;
    }

  public:
    FILE *recon_file_; /**< file handle to dave reconstruction video frames, set
                          it to public for accessable by create_recon_sink */

  protected:
    uint64_t max_frame_ts_; /**< maximun timestamp of current frames in list */
    std::vector<uint32_t> record_list_; /**< list of frame timstamp, to help
                                           check if the file is completed*/
};

class ReconSinkBufferSort_ASC {
  public:
    bool operator()(ReconSink::ReconMug *a, ReconSink::ReconMug *b) const {
        return a->time_stamp < b->time_stamp;
    };
};

class ReconSinkBuffer : public ReconSink {
  public:
    ReconSinkBuffer(VideoFrameParam fmt) : ReconSink(fmt) {
        sink_type_ = RECON_SINK_BUFFER;
        mug_list_.clear();
    }
    virtual ~ReconSinkBuffer() {
        while (mug_list_.size() > 0) {
            delete_mug(mug_list_.back());
            mug_list_.pop_back();
        }
    }
    void fill_mug(ReconMug *mug) override {
        if (mug->time_stamp < (uint64_t)frame_count_) {
            mug_list_.push_back(mug);
            std::sort(
                mug_list_.begin(), mug_list_.end(), ReconSinkBufferSort_ASC());
        } else  // drop the frames out of limitation
            delete_mug(mug);
    }
    const ReconMug *take_mug(const uint64_t time_stamp) override {
        for (ReconMug *mug : mug_list_) {
            if (mug->time_stamp == time_stamp)
                return mug;
        }
        return nullptr;
    }
    const ReconMug *take_mug_inorder(const uint32_t index) override {
        if (index < mug_list_.size())
            return mug_list_.at(index);
        return nullptr;
    }
    void pour_mug(ReconMug *mug) override {
        std::vector<ReconMug *>::iterator it =
            std::find(mug_list_.begin(), mug_list_.end(), mug);
        if (it != mug_list_.end()) {  // if the mug is in list
            delete_mug(*it);
            mug_list_.erase(it);
        } else  // only delete the mug not in list
            delete_mug(mug);
    }
    bool is_compelete() override {
        if (mug_list_.size() < frame_count_)
            return false;

        ReconMug *mug = mug_list_.at(frame_count_ - 1);
        if (mug == nullptr || mug->time_stamp != frame_count_ - 1)
            return false;
        return true;
    }

  protected:
    std::vector<ReconMug *> mug_list_; /**< list of frame containers */
};

ReconSink *create_recon_sink(const VideoFrameParam &param,
                             const char *file_path) {
    ReconSinkFile *new_sink = new ReconSinkFile(param, file_path);
    if (new_sink) {
        if (new_sink->recon_file_ == nullptr) {
            delete new_sink;
            new_sink = nullptr;
        }
    }
    return new_sink;
}

ReconSink *create_recon_sink(const VideoFrameParam &param) {
    return new ReconSinkBuffer(param);
}
