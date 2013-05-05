
#include "flowmaster.h"


/*
 * This is a simple C++ wrapper around the flowmaster library.
 * */

class flowmaster
{
public:
	flowmaster()
		:m_fm(fm_create())
	{
	}

	~flowmaster()
	{
		fm_destroy(m_fm);
	}

	fm_rc connect()
	{
		return fm_connect(m_fm);
	}

	fm_rc disconnect()
	{
		return fm_disconnect(m_fm);
	}

	fm_rc get_data(fm_data &data)
	{
		return fm_get_data(m_fm, *data);
	}

	fm_rc ping()
	{
		return fm_ping(m_fm);
	}

	fm_rc set_fan_speed(float duty_cycle)
	{
		return fm_set_fan_speed(m_fm, duty_cycle);
	}
	
	int set_pump_speed(float duty_cycle)
	{
		return fm_set_pump_speed(m_fm, duty_cycle);
	}

private:
	flowmaster *m_fm;

};
