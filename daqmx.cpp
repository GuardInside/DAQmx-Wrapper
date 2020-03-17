#include "daqmx.h"
#include <string>
#include <sstream>
#include <vector>
#include <memory>

using namespace NIDAQmx;

DAQException::DAQException(innards::int32 errorCode):
    std::runtime_error{ "Exception in NI-DAQmx library" },
    m_code{errorCode}
{
}

std::string DAQException::description() const
{
    std::string buffer;
    innards::int32 neededSize = innards::DAQmxGetErrorString(m_code, NULL, 0);
    if (neededSize > 0) {
        buffer.resize(neededSize);
        innards::DAQmxGetErrorString(m_code, &buffer[0], neededSize);
    }
    return buffer;
}

int DAQException::code() const
{
    return m_code;
}

Task::TaskHandle Task::CreateNamedTask(std::string name)
{
    TaskHandle retval;
    innards::int32 error = innards::DAQmxCreateTask(name.c_str(), &retval);
    if (error < 0)
        throw DAQException(error);

    return retval;
}

Task::~Task()
{
    innards::DAQmxClearTask(m_handle);
}

void Task::AddChannel(int device_num, int ai_port_measurement, int range)
{
    m_device_num = device_num;
    m_ai_port = ai_port_measurement;
    double limVal = 0.0;
    switch(range)
    {
        case 0: limVal = 10.0;      break;
        case 1: limVal = 5.0;       break;
        case 2: limVal = 0.5;       break;
        case 3: limVal = 0.05;      break;
        default:
            throw std::runtime_error("SetubChannel\nIncorrect range");
    }
    std::stringstream physicalName;
    physicalName << "/Dev" << m_device_num << "/ai" << m_ai_port;
    innards::int32 error = innards::DAQmxCreateAIVoltageChan(m_handle, physicalName.str().c_str(), NULL, DAQmx_Val_RSE, -limVal, limVal, DAQmx_Val_Volts, NULL);
    if (error < 0)
        throw DAQException(error);
}

size_t Task::GetChannelCount( void ) const
{
    innards::uInt32 chanCount;
    innards::int32 error = innards::DAQmxGetTaskNumChans(m_handle, &chanCount);
    if (error < 0)
        throw DAQException(error);
    return chanCount;
}

void Task::SetupFiniteAcquisition(double samplesPerSecond, double time_ms)
{
    m_bufferSize = samplesPerSecond * 1000 * time_ms;
    innards::int32 error = innards::DAQmxCfgSampClkTiming(m_handle, NULL, samplesPerSecond, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, m_bufferSize);
    if (error < 0)
        throw DAQException(error);
}

void Task::Start()
{
    innards::int32 error = innards::DAQmxStartTask(m_handle);
    if (error < 0)
        throw DAQException(error);
}

void Task::Stop()
{
    innards::int32 error = innards::DAQmxStopTask(m_handle);
    if (error < 0)
        throw DAQException(error);
}

size_t Task::TryRead(std::vector<double> *buffer, innards::bool32 fillMode)
{
    static const double fTimeout = 10.0; /* Seconds */
    std::unique_ptr<double[]> tmp_buffer{std::make_unique<double>(m_bufferSize)};
    buffer->reserve(m_bufferSize);
    innards::int32 samplesRead;
    innards::int32 error = innards::DAQmxReadAnalogF64(m_handle, DAQmx_Val_Auto, fTimeout, fillMode, tmp_buffer.get(), m_bufferSize, &samplesRead, NULL);
    if (error < 0)
        throw DAQException(error);
    buffer->assign(tmp_buffer.get(), tmp_buffer.get() + m_bufferSize);
    return samplesRead;
}

void Task::SetupTrigger(int trigger_port, int edge, double gateTime /*мкс*/)
{
    if(edge != DAQmx_Val_Falling && edge != DAQmx_Val_Rising)
        throw std::runtime_error("SetupTriggerEdge\nIncorrect edge");

    innards::int32 error = innards::DAQmxSetStartTrigDelayUnits(m_handle, DAQmx_Val_Seconds);
    if (error < 0)
        throw DAQException(error);

    error = innards::DAQmxSetStartTrigDelay(m_handle, 0.000001*gateTime);
    if (error < 0)
        throw DAQException(error);

    std::stringstream physicalName;
    physicalName << "/Dev" << m_device_num << "/PFI" << trigger_port;
    error = innards::DAQmxCfgDigEdgeStartTrig(m_handle, physicalName.str().data(), edge);
    if (error < 0)
        throw DAQException(error);
}
