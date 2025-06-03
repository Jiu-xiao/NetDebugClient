#include <QDateTime>
#include <limits.h>

#include "timebase.hpp"

namespace LibXR
{
    /**
     * @brief QTTimebase 类，用于获取 Qt 系统的时间基准。Provides a timebase for Qt systems.
     *
     */
    class QTTimebase : public Timebase
    {
    public:
        /**
         * @brief 获取当前时间戳（微秒级）。Returns the current timestamp in microseconds.
         *
         * @return TimestampUS
         */
        TimestampUS _get_microseconds()
        {
            // 获取当前时间
            qint64 ms = QDateTime::currentMSecsSinceEpoch();
            // 转换为微秒，并从起始时间（libxr_linux_start_time）中减去
            qint64 microseconds = ms * 1000; // 转换为微秒
            return static_cast<TimestampUS>(microseconds % UINT32_MAX);
        }

        /**
         * @brief 获取当前时间戳（毫秒级）。Returns the current timestamp in milliseconds.
         *
         * @return TimestampMS
         */
        TimestampMS _get_milliseconds()
        {
            // 获取当前时间（毫秒）
            qint64 ms = QDateTime::currentMSecsSinceEpoch();
            return static_cast<TimestampMS>(ms % UINT32_MAX);
        }
    };
} // namespace LibXR
