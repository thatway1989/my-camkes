
#ifndef LOG_H_
#define LOG_H_

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
