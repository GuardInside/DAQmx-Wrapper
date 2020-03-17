#ifndef DAQMX_H_INCLUDED
#define DAQMX_H_INCLUDED
#include <string>
#include <vector>
#include <stdexcept>

namespace NIDAQmx
{
    namespace innards
    {
        #include <NIDAQmx.h>
    }
    class DAQException: public std::runtime_error
    {
            innards::int32 m_code;
            DAQException& operator=(const DAQException&) = delete;
        public:
            DAQException(innards::int32 errorCode);
            int         code()          const;
            std::string description()   const;
    };

    class Task
    {
        typedef innards::TaskHandle TaskHandle;

        private:
            const TaskHandle m_handle;
            int m_ai_port = 0;
            int m_device_num = 0;
            innards::int32 m_bufferSize = 0;
        private:
            Task& operator=(const Task&) = delete;

            Task(TaskHandle handle)
                : m_handle(handle)
            {
            }
            TaskHandle CreateNamedTask(std::string name);
        public:
            Task(std::string name)
                : m_handle(CreateNamedTask(name))
            {
            }
            ~Task();
            void AddChannel(int device_num, int ai_port_measurement, int range);
            /* Возвращает число каналов, закрепленных за таском */
            size_t GetChannelCount( void ) const;
            /* Задает частоту дискретизации и время измерения в мс */
            void SetupFiniteAcquisition(double samplesPerSecond, double time_ms);
            void SetupTrigger(int trigger_port, int edge, double gateTime /*мкс*/);

            void Start();
            void Stop();
            size_t TryRead(std::vector<double> *buffer, innards::bool32 fillMode = DAQmx_Val_GroupByChannel);
    };
}

#endif // DAQMX_H_INCLUDED
