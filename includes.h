#pragma once

#include <stdio.h>
#include <iostream>
#include <string.h>

extern "C"
{
#include "libavformat\avformat.h"
#include "libavcodec\avcodec.h"
#include "libavformat\avio.h"

#include "libavutil\avutil.h"
#include "libavutil\avassert.h"
#include "libavutil\avstring.h"
#include "libavutil\frame.h"
#include "libavutil\opt.h"

#include "libswresample\swresample.h"
}
