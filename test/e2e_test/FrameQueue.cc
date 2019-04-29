/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file FrameQueue.cc
 *
 * @brief Impelmentation of reconstruction frame queue
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#include <stdio.h>
#include <vector>
#include <algorithm>
#include "FrameQueue.h"
#include "CompareTools.h"
#ifdef ENABLE_DEBUG_MONITOR
#include "VideoMonitor.h"
#endif

#if _WIN32
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#define FOPEN(f, s, m) fopen_s(&f, s, m)
#else
#define fseeko64 fseek
#define ftello64 ftell
#define FOPEN(f, s, m) f = fopen(s, m)
#endif

static void delete_container(FrameQueue::FrameContainer *container) {
    if (container) {
        if (container->container_buf) {
            delete[] container->container_buf;
        }
        delete container;
    }
}

using svt_av1_e2e_tools::compare_image;
class FrameQueueFile : public FrameQueue {
  public:
    FrameQueueFile(const VideoFrameParam &param, const char *file_path)
        : FrameQueue(param) {
        queue_type_ = FRAME_QUEUE_FILE;
        max_frame_ts_ = 0;
        FOPEN(recon_file_, file_path, "wb");
        record_list_.clear();
    }
    virtual ~FrameQueueFile() {
        if (recon_file_) {
            fflush(recon_file_);
            fclose(recon_file_);
            recon_file_ = nullptr;
        }
        record_list_.clear();
    }
    void fill_container(FrameContainer *container) override {
        if (recon_file_ && container->filled_size &&
            container->filled_size <= container->container_size &&
            container->time_stamp < (uint64_t)frame_count_) {
            if (container->time_stamp >=
                max_frame_ts_) {  // new frame is larger than max timestamp
                fseeko64(recon_file_, 0, SEEK_END);
                for (size_t i = max_frame_ts_; i < container->time_stamp + 1;
                     ++i) {
                    fwrite(container->container_buf,
                           1,
                           container->container_size,
                           recon_file_);
                }
                max_frame_ts_ = container->time_stamp;
            }

            rewind(recon_file_);
            uint64_t frameNum = container->time_stamp;
            while (frameNum > 0) {
                int ret =
                    fseeko64(recon_file_, container->filled_size, SEEK_CUR);
                if (ret != 0) {
                    return;
                }
                frameNum--;
            }
            fwrite(container->container_buf,
                   1,
                   container->filled_size,
                   recon_file_);
            fflush(recon_file_);
            record_list_.push_back((uint32_t)container->time_stamp);
        }
        delete_container(container);
    }
    const FrameContainer *take_container(const uint64_t time_stamp) override {
        if (recon_file_ == nullptr)
            return nullptr;

        FrameContainer *container = nullptr;
        fseeko64(recon_file_, 0, SEEK_END);
        int64_t actual_size = ftello64(recon_file_);
        if (actual_size > 0 &&
            ((uint64_t)actual_size) > ((time_stamp + 1) * frame_size_)) {
            int ret = fseeko64(recon_file_, time_stamp * frame_size_, 0);
            if (ret != 0) {
                // printf("Error in fseeko64  returnVal %i\n", ret);
                return nullptr;
            }
            container = get_empty_container();
            if (container) {
                size_t read_size = fread(
                    container->container_buf, 1, frame_size_, recon_file_);
                if (read_size <= 0) {
                    pour_container(container);
                    container = nullptr;
                } else {
                    container->filled_size = (uint32_t)read_size;
                    container->time_stamp = time_stamp;
                }
            }
        }

        return container;
    }
    const FrameContainer *take_container_inorder(
        const uint32_t index) override {
        return take_container(index);
    }
    void pour_container(FrameContainer *container) override {
        delete_container(container);
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
                          it to public for accessable by create_frame_queue */

  protected:
    uint64_t max_frame_ts_; /**< maximun timestamp of current frames in list */
    std::vector<uint32_t> record_list_; /**< list of frame timstamp, to help
                                           check if the file is completed*/
};

class FrameQueueBufferSort_ASC {
  public:
    bool operator()(FrameQueue::FrameContainer *a,
                    FrameQueue::FrameContainer *b) const {
        return a->time_stamp < b->time_stamp;
    };
};

class FrameQueueBuffer : public FrameQueue {
  public:
    FrameQueueBuffer(VideoFrameParam fmt) : FrameQueue(fmt) {
        queue_type_ = FRAME_QUEUE_BUFFER;
        container_list_.clear();
    }
    virtual ~FrameQueueBuffer() {
        while (container_list_.size() > 0) {
            delete_container(container_list_.back());
            container_list_.pop_back();
        }
    }
    void fill_container(FrameContainer *container) override {
        if (container->time_stamp < (uint64_t)frame_count_) {
            container_list_.push_back(container);
            std::sort(container_list_.begin(),
                      container_list_.end(),
                      FrameQueueBufferSort_ASC());
        } else  // drop the frames out of limitation
            delete_container(container);
    }
    const FrameContainer *take_container(const uint64_t time_stamp) override {
        for (FrameContainer *container : container_list_) {
            if (container->time_stamp == time_stamp)
                return container;
        }
        return nullptr;
    }
    const FrameContainer *take_container_inorder(
        const uint32_t index) override {
        if (index < container_list_.size())
            return container_list_.at(index);
        return nullptr;
    }
    void pour_container(FrameContainer *container) override {
        std::vector<FrameContainer *>::iterator it = std::find(
            container_list_.begin(), container_list_.end(), container);
        if (it != container_list_.end()) {  // if the container is in list
            delete_container(*it);
            container_list_.erase(it);
        } else  // only delete the container not in list
            delete_container(container);
    }
    bool is_compelete() override {
        if (container_list_.size() < frame_count_)
            return false;

        FrameContainer *container = container_list_.at(frame_count_ - 1);
        if (container == nullptr || container->time_stamp != frame_count_ - 1)
            return false;
        return true;
    }

  protected:
    std::vector<FrameContainer *>
        container_list_; /**< list of frame containers */
};

class RefQueue : public ICompareQueue, FrameQueueBuffer {
  public:
    RefQueue(VideoFrameParam fmt, FrameQueue *my_friend)
        : FrameQueueBuffer(fmt) {
        friend_ = my_friend;
        frame_vec_.clear();
#ifdef ENABLE_DEBUG_MONITOR
        recon_monitor_ = nullptr;
        ref_monitor_ = nullptr;
#endif
    }
    virtual ~RefQueue() {
        while (frame_vec_.size()) {
            const VideoFrame *p = frame_vec_.back();
            frame_vec_.pop_back();
            if (p) {
                // printf("Reference queue still remain frames when
                // delete(%u)\n",
                //       (uint32_t)p->timestamp);
                delete p;
            }
        }
        friend_ = nullptr;
#ifdef ENABLE_DEBUG_MONITOR
        if (recon_monitor_) {
            delete recon_monitor_;
            recon_monitor_ = nullptr;
        }
        if (ref_monitor_) {
            delete ref_monitor_;
            ref_monitor_ = nullptr;
        }
#endif
    }

  public:
    bool compare_video(const VideoFrame &frame) override {
        const FrameContainer *container =
            friend_->take_container(frame.timestamp);
        if (container) {
            draw_frames(&frame, container);
            bool is_same = compare_image(container, &frame, frame.format);
            if (!is_same) {
                printf("ref_frame(%u) compare failed!!\n",
                       (uint32_t)frame.timestamp);
            }
            return is_same;
        } else {
            clone_frame(frame);
        }
        return true; /**< default return suceess if not found recon frame */
    }
    bool flush_video() override {
        bool is_all_same = true;
        for (const VideoFrame *frame : frame_vec_) {
            const FrameContainer *container =
                friend_->take_container(frame->timestamp);
            if (container) {
                draw_frames(frame, container);
                if (!compare_image(container, frame, frame->format)) {
                    printf("ref_frame(%u) compare failed!!\n",
                           (uint32_t)frame->timestamp);
                    is_all_same = false;
                }
            }
        }
        return is_all_same;
    }

  private:
    void clone_frame(const VideoFrame &frame) {
        VideoFrame *new_frame = new VideoFrame(frame);
        if (new_frame)
            frame_vec_.push_back(new_frame);
        else
            printf("out of memory for clone video frame!!\n");
    }
    void draw_frames(const VideoFrame *frame, const FrameContainer *container) {
#ifdef ENABLE_DEBUG_MONITOR
        if (ref_monitor_ == nullptr) {
            ref_monitor_ = new VideoMonitor(frame->width,
                                            frame->height,
                                            frame->stride[0],
                                            frame->bits_per_sample,
                                            false,
                                            "Ref decode");
        }
        if (ref_monitor_) {
            ref_monitor_->draw_frame(
                frame->planes[0], frame->planes[1], frame->planes[2]);
        }
        // Output to monitor for debug
        if (recon_monitor_ == nullptr) {
            recon_monitor_ = new VideoMonitor(
                frame->width,
                frame->height,
                frame->width * (frame->bits_per_sample > 8 ? 2 : 1),
                frame->bits_per_sample,
                false,
                "Recon");
        }
        if (recon_monitor_) {
            int luma_len = frame->width * frame->height *
                           (frame->bits_per_sample > 8 ? 2 : 1);
            recon_monitor_->draw_frame(
                container->container_buf,
                container->container_buf + luma_len,
                container->container_buf + luma_len * 5 / 4);
        }
#endif
    }

  private:
    FrameQueue *friend_;
    std::vector<const VideoFrame *> frame_vec_;
#ifdef ENABLE_DEBUG_MONITOR
    VideoMonitor *recon_monitor_;
    VideoMonitor *ref_monitor_;
#endif
};

FrameQueue *create_frame_queue(const VideoFrameParam &param,
                               const char *file_path) {
    FrameQueueFile *new_queue = new FrameQueueFile(param, file_path);
    if (new_queue) {
        if (new_queue->recon_file_ == nullptr) {
            delete new_queue;
            new_queue = nullptr;
        }
    }
    return new_queue;
}

FrameQueue *create_frame_queue(const VideoFrameParam &param) {
    return new FrameQueueBuffer(param);
}

ICompareQueue *create_ref_compare_queue(const VideoFrameParam &param,
                                        FrameQueue *recon) {
    return new RefQueue(param, recon);
}
