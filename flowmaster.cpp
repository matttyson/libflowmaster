#include "flowmaster.hpp"
#include "flowmaster.h"

flowmaster::flowmaster()
	:m_fm(fm_create())
{

}

flowmaster::~flowmaster()
{
	fm_disconnect(m_fm);
	fm_destroy(m_fm);
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
	fm_update_status(m_fm);
}

int flowmaster::fan_rpm()
{
	return fm_fan_rpm(m_fm);
}

int flowmaster::pump_rpm()
{
	return fm_pump_rpm(m_fm);
}

double flowmaster::ambient_temp()
{
	return fm_ambient_temp(m_fm);
}

double flowmaster::coolant_temp()
{
	return fm_coolant_temp(m_fm);
}

double flowmaster::fan_duty_cycle()
{
	return fm_fan_duty_cycle(m_fm);
}

double flowmaster::pump_duty_cycle()
{
	return fm_pump_duty_cycle(m_fm);
}
