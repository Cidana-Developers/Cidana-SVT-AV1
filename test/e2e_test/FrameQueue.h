/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file FrameQueue.h
 *
 * @brief Defines a queue to collect reconstruction frames
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#ifndef _FRAME_QUEUE_H_
#define _FRAME_QUEUE_H_

#include <stdint.h>
#include <memory.h>
#include "VideoFrame.h"

/** FrameQueue is a class designed to collect YUV video frames. It provides
 * interfaces for generating, store and destory frame containers. It can be
 * implemented with file-mode or buffer-mode to store the video frames, and it
 * also provides inside sorting by timestamp.
 */
class FrameQueue {
  public:
    /** FrameQueueType is enumerate type of queue type, file or buffer mode */
    typedef enum FrameQueueType {
        FRAME_QUEUE_BUFFER,
        FRAME_QUEUE_FILE,
    } FrameQueueType;

    typedef struct FrameContainer {
        uint32_t tag;        /**< tag of the frame */
        uint64_t time_stamp; /**< timestamp of the frame, current should be the
                                index of display order*/
        uint32_t container_size; /**< size of the frame container buffer*/
        uint32_t filled_size;    /**< size of the actual data in buffer */
        uint8_t* container_buf;  /**< memory buffer of the frame */
    } FrameContainer;

  public:
    /** Constructor of FrameQueue
     * @param param the parameters of the video frame
     */
    FrameQueue(const VideoFrameParam& param) {
        queue_type_ = FRAME_QUEUE_BUFFER;
        video_param_ = param;
        frame_size_ = calculate_frame_size(video_param_);
        frame_count_ = 0;
    }
    /** Destructor of FrameQueue	  */
    virtual ~FrameQueue() {
    }
    /** Get queue type
     * @return
     * FrameQueueType -- the type of queue
     */
    FrameQueueType get_type() {
        return queue_type_;
    }
    /** Get video parameter
     * @return
     * VideoFrameParam -- the parameter of video frame
     */
    VideoFrameParam get_video_param() {
        return video_param_;
    }
    /** Get total frame count in queue
     * @return
     * uint32_t -- the count of frame in queue
     */
    uint32_t get_frame_count() {
        return frame_count_;
    }
    /** Get maximum video frame number in queue
     * @param count  the maximum video frame number
     */
    void set_frame_count(const uint32_t count) {
        frame_count_ = count;
    }
    /** Get an empty video frame container from queue
     * @return
     * FrameContainer -- a container of video frame
     * nullptr -- no available container
     */
    FrameContainer* get_empty_container() {
        FrameContainer* new_container = new FrameContainer;
        if (new_container) {
            memset(new_container, 0, sizeof(FrameContainer));
            new_container->container_size = frame_size_;
            new_container->container_buf = new uint8_t[frame_size_];
            if (new_container->container_buf == nullptr) {
                delete new_container;
                new_container = nullptr;
            }
        }
        return new_container;
    }
    /** Interface of insert a container into queue
     * @param container  the container to insert into queue
     */
    virtual void fill_container(FrameContainer* container) = 0;
    /** Interface of get a container with video frame by the same timestamp
     * @param time_stamp  the timestamp of video frame to retreive
     * @return
     * FrameContainer -- a container of video frame
     * nullptr -- no available container by this timestamp
     */
    virtual const FrameContainer* take_container(const uint64_t time_stamp) = 0;
    /** Interface of get a container with video frame by index
     * @param index  the index of container to retreive
     * @return
     * FrameContainer -- a container of video frame
     * nullptr -- no available container by index
     */
    virtual const FrameContainer* take_container_inorder(
        const uint32_t index) = 0;
    /** Interface of destroy a container and remove from queue
     * @param container  the container to distroy
     */
    virtual void pour_container(FrameContainer* container) = 0;
    /** Interface of get whether the queue is compeletely filled
     * @return
     * true -- the queue is filled
     * false -- the queue is still available
     */
    virtual bool is_compelete() = 0;

  protected:
    /** Tool of video frame size caculation, with width, height and bit-depth
     * @param param  parameter of video frame
     * @return
     * the size in byte of the video frame
     */
    static uint32_t calculate_frame_size(const VideoFrameParam& param) {
        uint32_t lumaSize = param.width * param.height;
        uint32_t chromaSize = 0;
        switch (param.format) {
        case IMG_FMT_420: chromaSize = lumaSize >> 2; break;
        case IMG_FMT_422: chromaSize = lumaSize >> 1; break;
        case IMG_FMT_444: chromaSize = lumaSize; break;
        case IMG_FMT_420P10_PACKED:
            lumaSize = lumaSize << 1;
            chromaSize = lumaSize >> 2;
            break;
        case IMG_FMT_422P10_PACKED:
            lumaSize = lumaSize << 1;
            chromaSize = lumaSize >> 1;
            break;
        case IMG_FMT_444P10_PACKED:
            lumaSize = lumaSize << 1;
            chromaSize = lumaSize;
            break;
        default: break;
        }
        return lumaSize + (2 * chromaSize);
    }

  protected:
    FrameQueueType queue_type_;   /**< type of queue*/
    VideoFrameParam video_param_; /**< video frame parameters*/
    uint32_t frame_size_;         /**< size of video frame*/
    uint32_t frame_count_;        /**< maximun number of video frames*/
};

class ICompareQueue {
  public:
    virtual ~ICompareQueue(){};
    virtual bool compare_video(const VideoFrame& frame) = 0;
    virtual bool flush_video() = 0;
};

/** Interface of create a queue of reconstruction video frame with video
 * parameters and the file path to store
 * @param param  the parameter of video frame
 * @param file_path  the file path to store the containers
 * @return
 * FrameQueue -- the queue created
 * nullptr -- creation failed
 */
FrameQueue* create_frame_queue(const VideoFrameParam& param,
                               const char* file_path);

/** Interface of create a queue of reconstruction video frame with video
 * parameters
 * @param param  the parameter of video frame
 * @return
 * FrameQueue -- the queue created
 * nullptr -- creation failed
 */
FrameQueue* create_frame_queue(const VideoFrameParam& param);

/** Interface of create a queue of reference frames to compare with recon
 * parameters
 * @param param  the parameter of video frame
 * @param recon  the queue of recon video frame
 * @return
 * FrameQueue -- the queue created
 * nullptr -- creation failed
 */
ICompareQueue* create_ref_compare_queue(const VideoFrameParam& param,
                                        FrameQueue* recon);

#endif  // !_FRAME_QUEUE_H_
