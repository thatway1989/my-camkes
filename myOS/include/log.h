
#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#define DEBUG_LOW       1u
#define DEBUG_MEDIUM    2u
#define DEBUG_HIGH      3u
#define DEBUG_NONE      4u

typedef enum
{
  LOG_ERROR = 1,
  LOG_WARNING,
  LOG_INFO,
  LOG_DEBUG,
  LOG_MAX
} Log_Type;

#define LOG_LEVEL LOG_MAX

#define LOGD(format, ...) \
	do { \
	 	if (LOG_DEBUG < LOG_LEVEL) \
			printf("[%s]:(%s,%d):"format"\n", LOG_TAG, __func__, __LINE__, ##__VA_ARGS__);  \
	} while(0)

#define LOGI(format, ...) \
	do { \
	 	if (LOG_DEBUG < LOG_LEVEL) \
			printf("[%s]:(%s,%d):"format"\n", LOG_TAG, __func__, __LINE__, ##__VA_ARGS__);  \
	} while(0)

#define LOGW(format, ...) \
	do { \
	 	if (LOG_DEBUG < LOG_LEVEL) \
			printf("[%s]:(%s,%d):"format"\n", LOG_TAG, __func__, __LINE__, ##__VA_ARGS__);  \
	} while(0)

#define LOGE(format, ...) \
	do { \
	 	if (LOG_DEBUG < LOG_LEVEL) \
			printf("[%s]:(%s,%d):"format"\n", LOG_TAG, __func__, __LINE__, ##__VA_ARGS__);  \
	} while(0)

#endif

#define DEBUG(_level,...) \
    do { \
        if(_level >= LOG_LEVEL) { \
            printf (__VA_ARGS__); \
        }; \
    } while(0);
