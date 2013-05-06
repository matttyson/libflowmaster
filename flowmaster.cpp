#include "flowmaster.hpp"
#include "flowmaster.h"

flowmaster::flowmaster()
	:m_fm(fm_create()),m_data(new fm_data)
{

}

flowmaster::~flowmaster()
{
	fm_disconnect(m_fm);
	fm_destroy(m_fm);
	delete m_data;
}

int flowmaster::connect(const char *port)
{
	fm_connect(m_fm, port);
}

int flowmaster::disconnect()
{
	fm_disconnect(m_fm);
}

int flowmaster::ping()
{
	return fm_ping(m_fm);
}

int flowmaster::set_fan_speed(double duty_cycle)
{
	return fm_set_fan_speed(m_fm, duty_cycle);
}

int flowmaster::set_pump_speed(double duty_cycle)
{
	return fm_set_pump_speed(m_fm, duty_cycle);
}

int flowmaster::do_update()
{
	fm_get_data(m_fm, m_data);
}

int flowmaster::fan_rpm()
{
	return m_data->fan_rpm;
}

int flowmaster::pump_rpm()
{
	return m_data->pump_rpm;
}

double flowmaster::ambient_temp()
{
	return m_data->ambient_temp;
}

double flowmaster::coolant_temp()
{
	return m_data->coolant_temp;
}

double flowmaster::fan_duty_cycle()
{
	return m_data->fan_duty_cycle;
}

double flowmaster::pump_duty_cycle()
{
	return m_data->pump_duty_cycle;
}
