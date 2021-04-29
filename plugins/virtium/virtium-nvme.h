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
            ENTRY("get-maverick-fw-version", "Prints custom firmware string for Maverick devices to the console", vt_get_maverick_fw_string)
            ENTRY("parse-maverick-telemetry", "Prints formatted Maverick-specific telemetry data block", vt_parse_maverick_telemetry)
    )
);

#endif

#include "define_cmd.h"
