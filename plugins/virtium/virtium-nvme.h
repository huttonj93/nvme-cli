#undef CMD_INC_FILE
#define CMD_INC_FILE plugins/virtium/virtium-nvme

#if !defined(VIRTIUM_NVME) || defined(CMD_HEADER_MULTI_READ)
#define VIRTIUM_NVME

#include "cmd.h"
#include "plugin.h"

PLUGIN(NAME("virtium", "Virtium vendor specific extensions"),
    COMMAND_LIST(
            ENTRY("save-smart-to-vtview-log", "Periodically save smart attributes into a log file.\n\
                             The data in this log file can be analyzed using excel or using Virtium’s vtView.\n\
                             Visit vtView.virtium.com to see full potential uses of the data", vt_save_smart_to_vtview_log)
            ENTRY("show-identify", "Shows detail features and current settings", vt_show_identify)
            ENTRY("get-series32-fw-version", "Prints custom firmware string for Series 32 devices to the console", vt_get_series32_fw_string)
            ENTRY("parse-series32-telemetry", "Prints formatted Series 32-specific telemetry data block", vt_parse_series32_telemetry)
            ENTRY("parse-series61-vs-info", "Prints formatted Series 61-specific info block (Log 0xC6h)", vt_parse_series61_vs_info)
            ENTRY("get-sn", "Prints the serial number string to the console", vt_get_sn_info)
            ENTRY("get-mn", "Prints the model number string to the console", vt_get_mn_info)
            ENTRY("get-event-log", "Prints the event log to file", vt_get_event_log)
	    ENTRY("get-crash-info", "Prings the crash info", vt_get_crash_info)
            ENTRY("get-crash-dump", "Prings the crash info", vt_get_crash_dump)
	    ENTRY("set-info", "set the model number or serial number", vt_set_info)
            ENTRY("get-op", "get the drive over provisioning percent", vt_get_op_info)
            ENTRY("set-op", "set the drive over provisioning percent", vt_set_op_info)
	    ENTRY("get-gen", "get the current PCIe generation info", vt_get_gen_info)
            ENTRY("set-gen", "set max PCIe generation to use", vt_set_gen_info)
            ENTRY("get-width", "get the pcie bus width info", vt_get_width_info)
            ENTRY("set-width", "set the pcie bus width info", vt_set_width_info)
            ENTRY("set-uart-control", "turn the uart control on or off", vt_set_uart_contrl)
	    ENTRY("reboot", "reset the drive", vt_reboot)
            ENTRY("set-psid", "set the psid", vt_set_psid)
            ENTRY("factory-defect", "get the factory defect", vt_get_factory_defect)
            ENTRY("grown-defect", "get the grown defect", vt_get_grown_defect)
            ENTRY("erase-count", "get the erase count", vt_get_erase_count)
    )
);

#endif

#include "define_cmd.h"
