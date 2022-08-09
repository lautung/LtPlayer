//
// Created by lautung on 2022/8/9.
//

#ifndef LTPLAYER_AUDIOCHANNEL_H
#define LTPLAYER_AUDIOCHANNEL_H


#include "BaseChannel.h"

class AudioChannel: public BaseChannel{

public:
    AudioChannel(int stream_index, AVCodecContext * codecContext);

    virtual ~AudioChannel();

    void start();

    void stop();
};


#endif //LTPLAYER_AUDIOCHANNEL_H
