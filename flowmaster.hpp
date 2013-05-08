
/*
 * This is a simple C++ wrapper around the flowmaster library.
 * */

struct flowmaster_s;
struct fm_data_s;

class flowmaster
{
public:
	flowmaster();
	~flowmaster();

	/*
	 *	port is the name of the serial port we are talking to
	 *	on linux this will be /dev/tty*.
	 *	on windows this will be COMx
	 * */
	int connect(const char *port);
	int disconnect();

	int ping();
	
	// Duty cycle is a floating point number between 0.0 and 1.0.
	int set_fan_speed(float duty_cycle);
	int set_pump_speed(float duty_cycle);

	// Query the flowmaster for new information about the system status
	// Do not call this method more than once every 500ms
	int do_update();

	// Call do_update() to refresh these values
	int fan_rpm();
	int pump_rpm();

	float ambient_temp();
	float coolant_temp();

	float fan_duty_cycle();
	float pump_duty_cycle();

private:
	flowmaster_s *m_fm;
};
