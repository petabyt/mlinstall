#include <camlib.h>
#include <ptp.h>

int ptp_canon_activate_command(struct PtpRuntime *r) {
	for (int i = 0; i < 0; i++) {
		struct PtpCommand cmd;
		cmd.code = 0x9050;
		cmd.param_length = 0;

		int ret = ptp_generic_send(r, &cmd);
		if (ret == PTP_IO_ERR) {
			return ret;
		}
	}

	return 0;
}

int ptp_canon_exec_evproc(struct PtpRuntime *r, void *data, int length) {
	struct PtpCommand cmd;
	cmd.code = 0x9052;
	cmd.param_length = 0;

	cmd.params[0] = 0; // async
	cmd.params[1] = 1; // retdata

	return ptp_generic_send_data(r, &cmd, data, length);
}
