
/*
 * This is a simple C++ wrapper around the flowmaster library.
 * */

#include "flowmaster.h"

#include <vector>


class flowmaster
{
public:
	flowmaster() :m_fm(fm_create()) { }
	~flowmaster() { fm_destroy(m_fm); }

	/*
	 *	port is the name of the serial port we are talking to
	 *	on linux this will be /dev/tty*.
	 *	on windows this will be COMx
	 * */
	int connect(const char *port) {
		return fm_connect(m_fm, port);
	}

	int disconnect() {
		return fm_disconnect(m_fm);
	}

	bool is_connected() {
		return (bool) fm_isconnected(m_fm);
	}

	int ping() {
		return fm_ping(m_fm);
	}

	// True - automatic mode
	// False - manual control
	int autoregulate(bool automatic) {
		return fm_autoregulate(m_fm, automatic ? 1 : 0);
	}

	// Duty cycle is a floating point number between 0.0 and 1.0.
	int set_fan_speed(double duty_cycle) {
		return fm_set_fan_speed(m_fm, duty_cycle);
	}
	int set_pump_speed(double duty_cycle) {
		return fm_set_pump_speed(m_fm, duty_cycle);
	}

	// Query the flowmaster for new information about the system status
	// Do not call this method more than once every 500ms
	int update_status() {
		return fm_update_status(m_fm);
	}

	// Call do_update() to refresh these values
	int fan_rpm() {
		return fm_fan_rpm(m_fm);
	}
	int pump_rpm() {
		return fm_pump_rpm(m_fm);
	}

	double ambient_temp() {
		return fm_ambient_temp(m_fm);
	}
	double coolant_temp() {
		return fm_coolant_temp(m_fm);
	}

	double fan_duty_cycle() {
		return fm_fan_duty_cycle(m_fm);
	}
	double pump_duty_cycle() {
		return fm_pump_duty_cycle(m_fm);
	}

	int set_fan_profile(const std::vector<float> &profile){
		int i = 0;
		float temparray[65];

		if(profile.size() != 65) {
			return -1;
		}

		for(const float data : profile) {
			temparray[i++] = data;
		}

		return fm_set_fan_profile(m_fm, temparray, sizeof(temparray) / sizeof(temparray[0]));
	}

	int get_fan_profile(std::vector<float> &profile) {
		float temparray[65];
		const int size = sizeof(temparray) / sizeof(temparray[0]);

		int rc = fm_get_fan_profile(m_fm, temparray, size);

		if(rc != FM_OK){
			return rc;
		}

		profile.clear();
		profile.reserve(size);

		for(int i = 0; i < size; i++){
			profile.push_back(temparray[i]);
		}

		return rc;
	}

private:
	struct flowmaster_s *m_fm;
};
