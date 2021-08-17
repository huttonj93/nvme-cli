#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>
#include <time.h>
#include <locale.h>

#include "linux/nvme_ioctl.h"
#include "nvme.h"
#include "nvme-print.h"
#include "nvme-ioctl.h"
#include "plugin.h"
#include "argconfig.h"
#include "suffix.h"

#define CREATE_CMD
#include "virtium-nvme.h"

#define MIN2(a, b) ( ((a) < (b))? (a) : (b))

#define HOUR_IN_SECONDS     3600

#define MAX_HEADER_BUFF     (20 * 1024)
#define MAX_LOG_BUFF        4096
#define DEFAULT_TEST_NAME   "Put the name of your test here"

static char vt_default_log_file_name[256];

struct vtview_log_header {
	char			path[256];
	char			test_name[256];
	long int		time_stamp;
	struct nvme_id_ctrl	raw_ctrl;
	struct nvme_firmware_log_page   raw_fw;
};

struct vtview_smart_log_entry {
	char	path[256];
	long int	time_stamp;
	struct nvme_id_ns	raw_ns;
	struct nvme_id_ctrl	raw_ctrl;
	struct nvme_smart_log	raw_smart;
};

struct vtview_save_log_settings {
	double	run_time_hrs;
	double	log_record_frequency_hrs;
	const char*	output_file;
	const char*	test_name;
};


// SM226X (series61) vendor specific log page 0xC6
typedef struct _series61_vs_logpage_c6h {
	uint64_t version8;
	uint64_t reallocSectorCount;
	uint64_t slcReallocSectorCount;
	uint64_t powerOnHoursCount;
	uint64_t uncorrectableErrorCount1;
	uint64_t uncorrectableErrorCount2;
	uint64_t eccUnc;
	uint64_t softLDPCCorrectionEventCount;
	uint64_t tlcReadRetryFail;
	uint64_t slcReadRetryFail;
	uint64_t temperature;
	uint64_t readRetryTrigger;
	uint64_t totalLBAsWritten;
	uint64_t totalLBAsRead;
	uint64_t ps3Count;
	uint64_t ps4Count;
	uint64_t l1_2Count;
	uint64_t tlcAvailSpareBlocks;
	uint64_t slcAvailSpareBlocks;
	uint64_t wearRangeDelta1;
	uint64_t wearRangeDelta2;
	uint64_t highTemp;
	uint64_t lowTemp;
	uint64_t avgTemp;
	uint64_t recentHigh;
	uint64_t recentLow;
	uint64_t autoCalibrationFail;
	uint64_t slcNandDataRead;
	uint64_t tlcNandDataRead;
	uint64_t validBlockCounts;
	uint64_t sclNandWrites;
	uint64_t tlcNandWrites;
	uint64_t tlc_qlcWearLevelingCount;
	uint64_t slcWearLevelingCount;
	uint64_t tlcNandWritesDueToWearLeveling;
	uint64_t slcNandWritesDueToWearLeveing;
	uint64_t currentTLCSpareSuperBlock;
	uint64_t currentSLCSpareSuperBlock;
	uint64_t slcToTLCDefragCount;
	uint64_t tlcDefragCount;
	uint64_t slcDefragCount;
	uint64_t readBackFailCount;
	uint64_t tlcProgramFail;
	uint64_t slcProgramFail;
	uint64_t tlcEraseFail;
	uint64_t slcEraseFail;
	uint64_t tlcEraseCycleAvg;
	uint64_t tlcEraseCycleMax;
	uint64_t tlcEraseCycleMin;
	uint64_t slcEraseCycleAvg;
	uint64_t slcEraseCycleMax;
	uint64_t slcEraseCycleMin;
	uint64_t ps0ToPs1Entry;
	uint64_t ps1ToPs0Exit;
	uint64_t ps1ToPs2Entry;
	uint64_t ps2ToPs1Exit;
	uint64_t ps2ToPsShutDownEntry;
	uint64_t linkDownshift;
	uint64_t idleSleepCount;
	uint64_t l1EventCount;
	uint64_t ecrcEventCount;
	uint64_t lcrcEventCount;
	uint64_t slc2tlcDataEvictionIdle;
	uint64_t slc2tlcDataEvictionRuntime;
	uint64_t dramEccCorrected;
	uint64_t sramEccCorrected;
	uint64_t e2eFailDetected;
	uint64_t ps3_5Count;
	uint64_t throttlingLightCount;
	uint64_t throttlingHeavyCount;
	uint64_t throttlingLightDuration;
	uint64_t throttlingHeavyDuration;
	uint64_t retryCount;
	uint64_t softDecodeCount;
	uint64_t manfBadBlocks;
	uint64_t manfBadBlocksWorst;
	uint64_t slcValidBlockCounts;
	uint64_t tlcValidBlockCounts;
	uint64_t slcSystemBlockWrites;
	uint64_t slcWritesDueToReadDistrub;
	uint64_t tlcWritesDueToReadDistrub;
	uint64_t maxOpenBlockSsgsCount;
	uint64_t maxClosedBlockSsgsCount;
	uint64_t l2pTableSwap;
	uint64_t ignore1;
	uint64_t slcMaxCwErrorCountWithRrPass;
	uint64_t tlc_qlcMaxCwErrorCountWithRrPass;
	uint64_t totalErasePoolBlockCount;
	uint64_t dslcBlocksInLinkTable;
	uint64_t maxDslcEraseCycleCount;
	uint64_t avgDslcEraseCycleCount;
	uint64_t dslcNandWrite;
	uint64_t dslcNandRead;
	uint64_t dslcToDslcDefragCount;
	uint64_t dslcProgramFail;
	uint64_t dslcEraseFail;
	uint64_t dslcRetiredBlocks;
	uint64_t dslcAllocSize;
	uint64_t reconditionCounts;
	uint64_t minDslcEraseCount;
	uint64_t maxTotalEraseCount;
	uint64_t minTotalEraseCount;
	uint64_t avgTotalEraseCount;
	uint64_t maxOpenSsgsCount;
	uint64_t maxClosedSsgsCount;
	uint64_t slcDummyWrites;
	uint64_t tlcDummyWrites;
	uint64_t dslcDummyWrites;
	uint64_t ignore2;
	uint64_t dummyReads;
	uint64_t coreDumpCount;
	uint64_t dataTrim;
	uint64_t ignore3[13];
	uint64_t tlcWaiWithDslcAsTlcSize;
	uint64_t tlcWaiWithDslcAsSlcSize;
	uint64_t slcWai;
	uint64_t tlcTotalEraseCount;
	uint64_t slcTotalEraseCount;
	uint64_t dslcTotalEraseCount;
	uint64_t tlcBlockSectorCount;
	uint64_t mapBlockPopCount;
	uint64_t gwProBlockPopCount;
	uint64_t dramRetrainCount;
} series61_vs_logpage_c6h;

// series32 telemetry data block description
typedef struct _mav_host_tele_log_data_t
{
    // Thermal info                         offset 0x200
    uint8_t  res0[16];
    uint16_t min_composite_temp;            ///< Minumum temperature in life of controller
    uint8_t  res1[2];
    uint16_t max_composite_temp;            ///< Maximum temperature in life of controller
    uint8_t  res2[10];
    uint32_t shutdown_transition_count;     ///< Enter shutdown mode count
    uint8_t  res3[4];
    uint8_t  max_thermal_state;             ///< Maximum thermal mode from power-on
    uint8_t  res4[7];
    // Bad info                             offset 0x230
    uint16_t num_valid_spare_block;         ///< Total available spare block count
    uint16_t num_initial_invalid_block;     ///< Total initial bad block count
    uint16_t run_time_bad_block;            ///< Total runtime block count
    uint16_t on_line_retired_block;         ///< Total retired block count that tigger by host read command
    uint16_t off_line_retired_block;        ///< Total retired block count that tigger by controller management
    uint8_t  res5[6];
    // PE Info                              offset 0x240
    uint32_t total_erase_count;             ///< Total erase Count that collect from NAND
    uint32_t max_erase_count;               ///< Maximum erase Count that collect from NAND
    uint32_t min_erase_count;               ///< Minimum erase Count that collect from NAND
    uint32_t avg_erase_count;               ///< Average erase Count that collect from NAND
    uint32_t slc_total_erase_count;         ///< Total erase Count of pSLC mode block that collect from NAND
    uint32_t slc_max_erase_count;           ///< Maximum erase Count of SLC mode block that collect from NAND
    uint32_t slc_min_erase_count;           ///< Minimum erase Count of SLC mode block that collect from NAND
    uint32_t slc_avg_erase_count;           ///< Average erase Count of SLC mode block that collect from NAND
    uint32_t tlc_total_erase_count;         ///< Total erase Count of TLC mode block that collect from NAND
    uint32_t tlc_max_erase_count;           ///< Maximum erase Count of TLC mode block that collect from NAND
    uint32_t tlc_min_erase_count;           ///< Minimum erase Count of TLC mode block that collect from NAND
    uint32_t tlc_avg_erase_count;           ///< Average erase Count of TLC mode block that collect from NAND
    uint8_t  res6[16];
    // Endurance Info                       offset 0x280
    uint8_t  slc_remaining_life_pe;         ///< Remain life percentage base on SLC used erase count
    uint8_t  slc_remaining_life_spare;      ///< Remain life percentage base on remain available spare count
    uint8_t  res7[4];
    uint8_t  tlc_remaining_life_pe;         ///< Remain life percentage base on TLC used erase count
    uint8_t  res8[9];
    uint64_t slc_total_write_count;         ///< Total SLC program sector count
    uint64_t tlc_total_write_count;         ///< Total TLC program sector count
    // Retry Info                           offset 0x2A0
    uint8_t  res9[4];
    uint32_t hw_entry_soft_decode;          ///< Total trigger to do LDPC soft decode count
    uint32_t hw_entry_raid_count;           ///< Total trigger to do RAID decode count
    uint32_t on_line_refresh_count;         ///< Total refresh block count that tigger by host read command
    uint32_t off_line_refresh_count;        ///< Total refresh block count that tigger by controller management
    uint32_t on_line_unc_chunk_count;       ///< Total UNC count that tigger by host read command
    uint32_t off_line_unc_chunk_count;      ///< Total UNC count that tigger by controller management
    uint16_t program_fail_blk_count;        ///< Total program failed block count
    uint16_t erase_fail_blk_count;          ///< Total erase failed block count
    uint16_t read_ecc_fail_blk_count;       ///< Total read ECC uncorrectable block count
    uint8_t  res10[14];

    uint8_t  res11[16];
    // Intelligent Scan Info                offset 0x2E0
    uint32_t on_line_scan_times;            ///< Total trigger to do scan count in device is doing read/write I/O 
    uint32_t on_line_scan_blkcnt;           ///< Total trigger to do scan block count in device is doing read/write I/O 
    uint32_t off_line_scan_times;           ///< Total trigger to do scan count in device is in idle
    uint32_t off_line_scan_blkcnt;          ///< Total trigger to do scan block count in device is in idle
    uint8_t  res12[16];
    // Error Info                           offset 0x300
    uint32_t read_unc_cmd_count;            ///< Total return UNC error read command count
    uint32_t pcie_fatal_err_count;          ///< Totel PCIe fatal error count that from PERST/NSSRT/FLR/Link RST happens in R/W
    uint32_t pcie_interface_downsft_count;  ///< Total PCIe speed downshift count that compare current speed to power-on speed
    uint32_t e2e_tag_err_count;             ///< Total return E2E error count
    uint16_t dram_one_bit_err_count;        ///< Total DRAM 1 bit error count
    uint16_t dram_two_bit_err_count;        ///< Total DRAM 2 bits error count
    uint16_t tsb_one_bit_err_count;         ///< Total SRAM 1 bit error count
    uint16_t tsb_two_bit_err_count;         ///< Total SRAM 2 bits error count
    uint8_t  res13[8];
    uint32_t vdt_27_fail_count;             ///< Total intial VDT detected 3.3V dropped to 2.7V count
    uint32_t pcie_gen1_link_count;          ///< Total PCIe link up in Gen1 count
    uint32_t pcie_gen2_link_count;          ///< Total PCIe link up in Gen2 count
    uint32_t pcie_gen3_link_count;          ///< Total PCIe link up in Gen3 count
    uint64_t ecrc_event_count;              ///< Total PCIe ECRC happen count
    uint64_t lcrc_event_count;              ///< Total PCIe LCRC happen count
    uint32_t pcie_gen4_link_count;          ///< Total PCIe link up in Gen4 count

    uint8_t  res14[28];                     // offset 0x340
    // System Info                          offset 0x360
    uint8_t  res15[12];
    uint8_t  pcie_link_speed;               ///< Current PCIe link up speed
    uint8_t  pcie_link_width;               ///< Current PCIe link up width
    uint8_t  res16[2];

    uint8_t  res17[16];
    // New Defined Info                     offset 0x380
    uint8_t  nand_temp[32];                 ///< Current all NAND dies temperature;
    uint32_t pcie_error_reg0;               ///< PCIe error register
    uint32_t pciex1_link_count;             ///< Total PCIe link up in width=x1 count
    uint32_t pciex2_link_count;             ///< Total PCIe link up in width=x2 count
    uint32_t pciex4_link_count;             ///< Total PCIe link up in width=x4 count
} mav_host_tele_log_data_t;

static void print_series61_log_c6h(series61_vs_logpage_c6h* data_block) {
	printf("version8: ");
	uint64_t version8 = data_block->version8;
	for (int i = 0; i < 8; i++) {
		printf("%d", (char) ((version8 & 0xff00000000000000) >> 56));
		version8 = version8 << 8;
	}
	printf("\n");
    printf("reallocSectorCount: %ld\n", data_block->reallocSectorCount);
    printf("slcReallocSectorCount: %ld\n", data_block->slcReallocSectorCount);
    printf("powerOnHoursCount: %ld\n", data_block->powerOnHoursCount);
    printf("uncorrectableErrorCount1: %ld\n", data_block->uncorrectableErrorCount1);
    printf("uncorrectableErrorCount2: %ld\n", data_block->uncorrectableErrorCount2);
    printf("eccUnc: %ld\n", data_block->eccUnc);
    printf("softLDPCCorrectionEventCount: %ld\n", data_block->softLDPCCorrectionEventCount);
    printf("tlcReadRetryFail: %ld\n", data_block->tlcReadRetryFail);
    printf("slcReadRetryFail: %ld\n", data_block->slcReadRetryFail);
    printf("temperature: %ld\n", data_block->temperature);
    printf("readRetryTrigger: %ld\n", data_block->readRetryTrigger);
    printf("totalLBAsWritten: %ld\n", data_block->totalLBAsWritten);
    printf("totalLBAsRead: %ld\n", data_block->totalLBAsRead);
    printf("ps3Count: %ld\n", data_block->ps3Count);
    printf("ps4Count: %ld\n", data_block->ps4Count);
    printf("l1_2Count: %ld\n", data_block->l1_2Count);
    printf("tlcAvailSpareBlocks: %ld\n", data_block->tlcAvailSpareBlocks);
    printf("slcAvailSpareBlocks: %ld\n", data_block->slcAvailSpareBlocks);
    printf("wearRangeDelta1: %ld\n", data_block->wearRangeDelta1);
    printf("wearRangeDelta2: %ld\n", data_block->wearRangeDelta2);
    printf("highTemp: %ld\n", data_block->highTemp);
    printf("lowTemp: %ld\n", data_block->lowTemp);
    printf("avgTemp: %ld\n", data_block->avgTemp);
    printf("recentHigh: %ld\n", data_block->recentHigh);
    printf("recentLow: %ld\n", data_block->recentLow);
    printf("autoCalibrationFail: %ld\n", data_block->autoCalibrationFail);
    printf("slcNandDataRead: %ld\n", data_block->slcNandDataRead);
    printf("tlcNandDataRead: %ld\n", data_block->tlcNandDataRead);
    printf("validBlockCounts: %ld\n", data_block->validBlockCounts);
    printf("sclNandWrites: %ld\n", data_block->sclNandWrites);
    printf("tlcNandWrites: %ld\n", data_block->tlcNandWrites);
    printf("tlc_qlcWearLevelingCount: %ld\n", data_block->tlc_qlcWearLevelingCount);
    printf("slcWearLevelingCount: %ld\n", data_block->slcWearLevelingCount);
    printf("tlcNandWritesDueToWearLeveling: %ld\n", data_block->tlcNandWritesDueToWearLeveling);
    printf("slcNandWritesDueToWearLeveing: %ld\n", data_block->slcNandWritesDueToWearLeveing);
    printf("currentTLCSpareSuperBlock: %ld\n", data_block->currentTLCSpareSuperBlock);
    printf("currentSLCSpareSuperBlock: %ld\n", data_block->currentSLCSpareSuperBlock);
    printf("slcToTLCDefragCount: %ld\n", data_block->slcToTLCDefragCount);
    printf("tlcDefragCount: %ld\n", data_block->tlcDefragCount);
    printf("slcDefragCount: %ld\n", data_block->slcDefragCount);
    printf("readBackFailCount: %ld\n", data_block->readBackFailCount);
    printf("tlcProgramFail: %ld\n", data_block->tlcProgramFail);
    printf("slcProgramFail: %ld\n", data_block->slcProgramFail);
    printf("tlcEraseFail: %ld\n", data_block->tlcEraseFail);
    printf("slcEraseFail: %ld\n", data_block->slcEraseFail);
    printf("tlcEraseCycleAvg: %ld\n", data_block->tlcEraseCycleAvg);
    printf("tlcEraseCycleMax: %ld\n", data_block->tlcEraseCycleMax);
    printf("tlcEraseCycleMin: %ld\n", data_block->tlcEraseCycleMin);
    printf("slcEraseCycleAvg: %ld\n", data_block->slcEraseCycleAvg);
    printf("slcEraseCycleMax: %ld\n", data_block->slcEraseCycleMax);
    printf("slcEraseCycleMin: %ld\n", data_block->slcEraseCycleMin);
    printf("ps0ToPs1Entry: %ld\n", data_block->ps0ToPs1Entry);
    printf("ps1ToPs0Exit: %ld\n", data_block->ps1ToPs0Exit);
    printf("ps1ToPs2Entry: %ld\n", data_block->ps1ToPs2Entry);
    printf("ps2ToPs1Exit: %ld\n", data_block->ps2ToPs1Exit);
    printf("ps2ToPsShutDownEntry: %ld\n", data_block->ps2ToPsShutDownEntry);
    printf("linkDownshift: %ld\n", data_block->linkDownshift);
    printf("idleSleepCount: %ld\n", data_block->idleSleepCount);
    printf("l1EventCount: %ld\n", data_block->l1EventCount);
    printf("ecrcEventCount: %ld\n", data_block->ecrcEventCount);
    printf("lcrcEventCount: %ld\n", data_block->lcrcEventCount);
    printf("slc2tlcDataEvictionIdle: %ld\n", data_block->slc2tlcDataEvictionIdle);
    printf("slc2tlcDataEvictionRuntime: %ld\n", data_block->slc2tlcDataEvictionRuntime);
    printf("dramEccCorrected: %ld\n", data_block->dramEccCorrected);
    printf("sramEccCorrected: %ld\n", data_block->sramEccCorrected);
    printf("e2eFailDetected: %ld\n", data_block->e2eFailDetected);
    printf("ps3_5Count: %ld\n", data_block->ps3_5Count);
    printf("throttlingLightCount: %ld\n", data_block->throttlingLightCount);
    printf("throttlingHeavyCount: %ld\n", data_block->throttlingHeavyCount);
    printf("throttlingLightDuration: %ld\n", data_block->throttlingLightDuration);
    printf("throttlingHeavyDuration: %ld\n", data_block->throttlingHeavyDuration);
    printf("retryCount: %ld\n", data_block->retryCount);
    printf("softDecodeCount: %ld\n", data_block->softDecodeCount);
    printf("manfBadBlocks: %ld\n", data_block->manfBadBlocks);
    printf("manfBadBlocksWorst: %ld\n", data_block->manfBadBlocksWorst);
    printf("slcValidBlockCounts: %ld\n", data_block->slcValidBlockCounts);
    printf("tlcValidBlockCounts: %ld\n", data_block->tlcValidBlockCounts);
    printf("slcSystemBlockWrites: %ld\n", data_block->slcSystemBlockWrites);
    printf("slcWritesDueToReadDistrub: %ld\n", data_block->slcWritesDueToReadDistrub);
    printf("tlcWritesDueToReadDistrub: %ld\n", data_block->tlcWritesDueToReadDistrub);
    printf("maxOpenBlockSsgsCount: %ld\n", data_block->maxOpenBlockSsgsCount);
    printf("maxClosedBlockSsgsCount: %ld\n", data_block->maxClosedBlockSsgsCount);
    printf("l2pTableSwap: %ld\n", data_block->l2pTableSwap);
    printf("slcMaxCwErrorCountWithRrPass: %ld\n", data_block->slcMaxCwErrorCountWithRrPass);
    printf("tlc_qlcMaxCwErrorCountWithRrPass: %ld\n", data_block->tlc_qlcMaxCwErrorCountWithRrPass);
    printf("totalErasePoolBlockCount: %ld\n", data_block->totalErasePoolBlockCount);
    printf("dslcBlocksInLinkTable: %ld\n", data_block->dslcBlocksInLinkTable);
    printf("maxDslcEraseCycleCount: %ld\n", data_block->maxDslcEraseCycleCount);
    printf("avgDslcEraseCycleCount: %ld\n", data_block->avgDslcEraseCycleCount);
    printf("dslcNandWrite: %ld\n", data_block->dslcNandWrite);
    printf("dslcNandRead: %ld\n", data_block->dslcNandRead);
    printf("dslcToDslcDefragCount: %ld\n", data_block->dslcToDslcDefragCount);
    printf("dslcProgramFail: %ld\n", data_block->dslcProgramFail);
    printf("dslcEraseFail: %ld\n", data_block->dslcEraseFail);
    printf("dslcRetiredBlocks: %ld\n", data_block->dslcRetiredBlocks);
    printf("dslcAllocSize: %ld\n", data_block->dslcAllocSize);
    printf("reconditionCounts: %ld\n", data_block->reconditionCounts);
    printf("minDslcEraseCount: %ld\n", data_block->minDslcEraseCount);
    printf("maxTotalEraseCount: %ld\n", data_block->maxTotalEraseCount);
    printf("minTotalEraseCount: %ld\n", data_block->minTotalEraseCount);
    printf("avgTotalEraseCount: %ld\n", data_block->avgTotalEraseCount);
    printf("maxOpenSsgsCount: %ld\n", data_block->maxOpenSsgsCount);
    printf("maxClosedSsgsCount: %ld\n", data_block->maxClosedSsgsCount);
    printf("slcDummyWrites: %ld\n", data_block->slcDummyWrites);
    printf("tlcDummyWrites: %ld\n", data_block->tlcDummyWrites);
    printf("dslcDummyWrites: %ld\n", data_block->dslcDummyWrites);
    printf("dummyReads: %ld\n", data_block->dummyReads);
    printf("coreDumpCount: %ld\n", data_block->coreDumpCount);
    printf("dataTrim: %ld\n", data_block->dataTrim);
    printf("tlcWaiWithDslcAsTlcSize: %ld\n", data_block->tlcWaiWithDslcAsTlcSize);
    printf("tlcWaiWithDslcAsSlcSize: %ld\n", data_block->tlcWaiWithDslcAsSlcSize);
    printf("slcWai: %ld\n", data_block->slcWai);
    printf("tlcTotalEraseCount: %ld\n", data_block->tlcTotalEraseCount);
    printf("slcTotalEraseCount: %ld\n", data_block->slcTotalEraseCount);
    printf("dslcTotalEraseCount: %ld\n", data_block->dslcTotalEraseCount);
    printf("tlcBlockSectorCount: %ld\n", data_block->tlcBlockSectorCount);
    printf("mapBlockPopCount: %ld\n", data_block->mapBlockPopCount);
    printf("gwProBlockPopCount: %ld\n", data_block->gwProBlockPopCount);
    printf("dramRetrainCount: %ld\n", data_block->dramRetrainCount);
}

static void print_tel_data_block(mav_host_tele_log_data_t* data_block)
{
	printf("min composite temp: %d\n", data_block->min_composite_temp);
	printf("max composite temp: %d\n", data_block->max_composite_temp);
	printf("shutdown transition count: %d\n", data_block->shutdown_transition_count);
	printf("max thermal state: %d\n", data_block->max_thermal_state);
	printf("total spare block count: %d\n", data_block->num_valid_spare_block);
	printf("total initial bad block count: %d\n", data_block->num_initial_invalid_block);
	printf("total runtime bad block count: %d\n", data_block->run_time_bad_block);
	printf("on line retired block: %d\n", data_block->on_line_retired_block);
	printf("off line retired block: %d\n", data_block->off_line_retired_block);
	printf("total erase count: %d\n", data_block->total_erase_count);
	printf("max erase count: %d\n", data_block->max_erase_count);
	printf("min erase count: %d\n", data_block->min_erase_count);
	printf("avg erase count: %d\n", data_block->avg_erase_count);
	printf("slc total erase count: %d\n", data_block->slc_total_erase_count);
	printf("slc max erase count: %d\n", data_block->slc_max_erase_count);
	printf("slc min erase count: %d\n", data_block->slc_min_erase_count);
	printf("slc avg erase count: %d\n", data_block->slc_avg_erase_count);
	printf("tlc total erase count: %d\n", data_block->tlc_total_erase_count);
	printf("tlc max erase count: %d\n", data_block->tlc_max_erase_count);
	printf("tlc min erase count: %d\n", data_block->tlc_min_erase_count);
	printf("tlc avg erase count: %d\n", data_block->tlc_avg_erase_count);
	printf("percent slc remaining life pe: %d\n", data_block->slc_remaining_life_pe);
	printf("percent slc remaining life spare count: %d\n", data_block->slc_remaining_life_spare);
	printf("percent tlc remaining life pe: %d\n", data_block->tlc_remaining_life_pe);
	printf("total slc progam sector count: %ld\n", data_block->slc_total_write_count);
	printf("total tlc progam sector count: %ld\n", data_block->tlc_total_write_count);
	printf("hw entry soft decode: %d\n", data_block->hw_entry_soft_decode);
	printf("hw entry raid count: %d\n", data_block->hw_entry_raid_count);
	printf("on line refresh count: %d\n", data_block->on_line_refresh_count);
	printf("off line refresh count: %d\n", data_block->off_line_refresh_count);
	printf("on line unc chunk count: %d\n", data_block->on_line_unc_chunk_count);
	printf("off line unc chunk count: %d\n", data_block->off_line_unc_chunk_count);
	printf("program fail block count: %d\n", data_block->program_fail_blk_count);
	printf("erase fail blk count: %d\n", data_block->erase_fail_blk_count);
	printf("Total read ECC uncorrectable block count: %d\n", data_block->read_ecc_fail_blk_count);
	printf("on line scan times: %d\n", data_block->on_line_scan_times);
	printf("on line scan block count: %d\n", data_block->on_line_scan_blkcnt);
	printf("off line scan times: %d\n", data_block->off_line_scan_times);
	printf("off line scan block count: %d\n", data_block->off_line_scan_blkcnt);
	printf("unc error read command count: %d\n", data_block->read_unc_cmd_count);
	printf("pcie fatal error count: %d\n", data_block->pcie_fatal_err_count);
	printf("pcie interface downshift count: %d\n", data_block->pcie_interface_downsft_count);
	printf("e2e error count: %d\n", data_block->e2e_tag_err_count);
	printf("dram 1 bit error count: %d\n", data_block->dram_one_bit_err_count);
	printf("dram 2 bit error count: %d\n", data_block->dram_two_bit_err_count);
	printf("sram 1 bit error count: %d\n", data_block->tsb_one_bit_err_count);
	printf("sram 2 bit error count: %d\n", data_block->tsb_two_bit_err_count);
	printf("Total initial VDT detected 3.3V dropped to 2.7V count: %d\n", data_block->vdt_27_fail_count);
	printf("PCIe link up in Gen1 count: %d\n", data_block->pcie_gen1_link_count);
	printf("PCIe link up in Gen2 count: %d\n", data_block->pcie_gen2_link_count);
	printf("PCIe link up in Gen3 count: %d\n", data_block->pcie_gen3_link_count);
	printf("ecrc event count: %ld\n", data_block->ecrc_event_count);
	printf("lcrc event count: %ld\n", data_block->lcrc_event_count);
	printf("PCIe link up in Gen3 count: %d\n", data_block->pcie_gen4_link_count);
	printf("Current PCIe link up speed: %d\n", data_block->pcie_link_speed);
	printf("Current PCIe link up width: %d\n", data_block->pcie_link_width);

	printf("All NAND die temps: ");
	for (int i = 0; i < 32; i++) {
		printf("%d ", data_block->nand_temp[i]);
	}
	printf("\n");

	printf("PCIe error register: %d\n", data_block->pcie_error_reg0);
	printf("PCIe link up in width=x1 count: %d\n", data_block->pciex1_link_count);
	printf("PCIe link up in width=x2 count: %d\n", data_block->pciex2_link_count);
	printf("PCIe link up in width=x4 count: %d\n", data_block->pciex4_link_count);
}

static long double int128_to_double(__u8 *data)
{
	int i;
	long double result = 0;

	for (i = 0; i < 16; i++) {
		result *= 256;
		result += data[15 - i];
	}
	return result;
}

static void vt_initialize_header_buffer(struct vtview_log_header *pbuff)
{
	memset(pbuff->path, 0, sizeof(pbuff->path));
	memset(pbuff->test_name, 0, sizeof(pbuff->test_name));
}

static void vt_convert_data_buffer_to_hex_string(const unsigned char *bufPtr,
		const unsigned int size, const bool isReverted, char *output)
{
	unsigned int i, pos;
	const char hextable[16] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F',
	};

	memset(output, 0, (size * 2) + 1);

	for (i = 0; i < size; i++) {
		if(isReverted)
			pos = size - 1 - i;
		else
			pos = i;
		output[2 * i] = hextable[(bufPtr[pos] & 0xF0) >> 4];
		output[2 * i + 1] = hextable[(bufPtr[pos] & 0x0F)];
	}
}

/*
 * Generate log file name.
 * Log file name will be generated automatically if user leave log file option blank.
 * Log file name will be generated as vtView-Smart-log-date-time.txt
 */
static void vt_generate_vtview_log_file_name(char* fname)
{
	time_t     current;
	struct tm  tstamp;
	char       temp[256];

	time(&current);

	tstamp = *localtime(&current);
	snprintf(temp, sizeof(temp), "./vtView-Smart-log-");
	strcat(fname, temp);
	strftime(temp, sizeof(temp), "%Y-%m-%d", &tstamp);
	strcat(fname, temp);
	snprintf(temp, sizeof(temp), ".txt");
	strcat(fname, temp);
}

static void vt_convert_smart_data_to_human_readable_format(struct vtview_smart_log_entry *smart, char *text)
{
	char tempbuff[1024] = "";
	int i;
	int temperature = ((smart->raw_smart.temperature[1] << 8) | smart->raw_smart.temperature[0]) - 273;
	double capacity;
	char *curlocale;
	char *templocale;

	curlocale = setlocale(LC_ALL, NULL);
	templocale = strdup(curlocale);

	if (NULL == templocale)
		printf("Cannot malloc buffer\n");

	setlocale(LC_ALL, "C");

	long long int lba = 1 << smart->raw_ns.lbaf[(smart->raw_ns.flbas & 0x0f)].ds;
	capacity = le64_to_cpu(smart->raw_ns.nsze) * lba;

	snprintf(tempbuff, sizeof(tempbuff), "log;%s;%lu;%s;%s;%-.*s;", smart->raw_ctrl.sn, smart->time_stamp, smart->path,
		smart->raw_ctrl.mn, (int)sizeof(smart->raw_ctrl.fr), smart->raw_ctrl.fr);
	strcpy(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Capacity;%lf;", capacity / 1000000000);
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Critical_Warning;%u;", smart->raw_smart.critical_warning);
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Temperature;%u;", temperature);
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Available_Spare;%u;", smart->raw_smart.avail_spare);
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Available_Spare_Threshold;%u;", smart->raw_smart.spare_thresh);
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Percentage_Used;%u;", smart->raw_smart.percent_used);
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Data_Units_Read;%0.Lf;", int128_to_double(smart->raw_smart.data_units_read));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Data_Units_Written;%0.Lf;", int128_to_double(smart->raw_smart.data_units_written));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Host_Read_Commands;%0.Lf;", int128_to_double(smart->raw_smart.host_reads));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Host_Write_Commands;%0.Lf;", int128_to_double(smart->raw_smart.host_writes));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Controller_Busy_Time;%0.Lf;", int128_to_double(smart->raw_smart.ctrl_busy_time));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Power_Cycles;%0.Lf;", int128_to_double(smart->raw_smart.power_cycles));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Power_On_Hours;%0.Lf;", int128_to_double(smart->raw_smart.power_on_hours));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Unsafe_Shutdowns;%0.Lf;", int128_to_double(smart->raw_smart.unsafe_shutdowns));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Media_Errors;%0.Lf;", int128_to_double(smart->raw_smart.media_errors));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Num_Err_Log_Entries;%0.Lf;", int128_to_double(smart->raw_smart.num_err_log_entries));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Warning_Temperature_Time;%u;", le32_to_cpu(smart->raw_smart.warning_temp_time));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Critical_Composite_Temperature_Time;%u;", le32_to_cpu(smart->raw_smart.critical_comp_time));
	strcat(text, tempbuff);

	for (i = 0; i < 8; i++) {
		__s32 temp = le16_to_cpu(smart->raw_smart.temp_sensor[i]);
		if (0 == temp) {
			snprintf(tempbuff, sizeof(tempbuff), "Temperature_Sensor_%d;NC;", i);
			strcat(text, tempbuff);
			continue;
		}
		snprintf(tempbuff, sizeof(tempbuff), "Temperature_Sensor_%d;%d;", i, temp - 273);
		strcat(text, tempbuff);
	}

	snprintf(tempbuff, sizeof(tempbuff), "Thermal_Management_T1_Trans_Count;%u;", le32_to_cpu(smart->raw_smart.thm_temp1_trans_count));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Thermal_Management_T2_Trans_Count;%u;", le32_to_cpu(smart->raw_smart.thm_temp2_trans_count));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Thermal_Management_T1_Total_Time;%u;", le32_to_cpu(smart->raw_smart.thm_temp1_total_time));
	strcat(text, tempbuff);
	snprintf(tempbuff, sizeof(tempbuff), "Thermal_Management_T2_Total_Time;%u;", le32_to_cpu(smart->raw_smart.thm_temp2_total_time));
	strcat(text, tempbuff);

	snprintf(tempbuff, sizeof(tempbuff), "NandWrites;%d;\n", 0);
	strcat(text, tempbuff);

	setlocale(LC_ALL, templocale);
	free(templocale);
}

static void vt_header_to_string(const struct vtview_log_header *header, char *text)
{
	char timebuff[50] = "";
	char tempbuff[MAX_HEADER_BUFF] = "";
	char identext[16384] = "";
	char fwtext[2048] = "";

	strftime(timebuff, 50, "%Y-%m-%d %H:%M:%S", localtime(&(header->time_stamp)));
	snprintf(tempbuff, MAX_HEADER_BUFF, "header;{\"session\":{\"testName\":\"%s\",\"dateTime\":\"%s\"},",
		header->test_name, timebuff);
	strcpy(text, tempbuff);

	vt_convert_data_buffer_to_hex_string((unsigned char *)&(header->raw_ctrl), sizeof(header->raw_ctrl), false, identext);
	vt_convert_data_buffer_to_hex_string((unsigned char *)&(header->raw_fw), sizeof(header->raw_fw), false, fwtext);
	snprintf(tempbuff, MAX_HEADER_BUFF,
		"\"devices\":[{\"model\":\"%s\",\"port\":\"%s\",\"SN\":\"%s\",\"type\":\"NVMe\",\"identify\":\"%s\",\"firmwareSlot\":\"%s\"}]}\n",
		header->raw_ctrl.mn, header->path, header->raw_ctrl.sn, identext, fwtext);
	strcat(text, tempbuff);
}

static int vt_append_text_file(const char *text, const char *filename)
{
	FILE *f;

	f = fopen(filename, "a");
	if(NULL == f) {
		printf("Cannot open %s\n", filename);
		return -1;
	}

	fprintf(f, "%s", text);
	fclose(f);
	return 0;
}

static int vt_append_log(struct vtview_smart_log_entry *smart, const char *filename)
{
	char sm_log_text[MAX_LOG_BUFF] = "";

	vt_convert_smart_data_to_human_readable_format(smart, sm_log_text);
	return vt_append_text_file(sm_log_text, filename);
}

static int vt_append_header(const struct vtview_log_header *header, const char *filename)
{
	char header_text[MAX_HEADER_BUFF] = "";

	vt_header_to_string(header, header_text);
	return vt_append_text_file(header_text, filename);
}

static void vt_process_string(char *str, const size_t size)
{
	size_t i;

	if (size == 0)
		return;

	i = size - 1;
	while ((0 != i) && (' ' == str[i])) {
		str[i] = 0;
		i--;
	}
}

static int vt_add_entry_to_log(const int fd, const char *path, const struct vtview_save_log_settings *cfg)
{
	struct vtview_smart_log_entry smart;
	char filename[256] = "";
	int ret = 0;
	int nsid = 0;

	memset(smart.path, 0, sizeof(smart.path));
	strcpy(smart.path, path);
	if(NULL == cfg->output_file)
		strcpy(filename, vt_default_log_file_name);
	else
		strcpy(filename, cfg->output_file);

	smart.time_stamp = time(NULL);
	nsid = nvme_get_nsid(fd);

	if (nsid <= 0) {
		printf("Cannot read namespace-id\n");
		return -1;
	}

	ret = nvme_identify_ns(fd, nsid, 0, &smart.raw_ns);
	if (ret) {
		printf("Cannot read namespace identify\n");
		return -1;
	}

	ret = nvme_identify_ctrl(fd, &smart.raw_ctrl);
	if (ret) {
		printf("Cannot read device identify controller\n");
		return -1;
	}

	ret = nvme_smart_log(fd, NVME_NSID_ALL, &smart.raw_smart);
	if (ret) {
		printf("Cannot read device SMART log\n");
		return -1;
	}

	vt_process_string(smart.raw_ctrl.sn, sizeof(smart.raw_ctrl.sn));
	vt_process_string(smart.raw_ctrl.mn, sizeof(smart.raw_ctrl.mn));

	ret = vt_append_log(&smart, filename);
	return (ret);
}

static int vt_update_vtview_log_header(const int fd, const char *path, const struct vtview_save_log_settings *cfg)
{
	struct vtview_log_header header;
	char filename[256] = "";
	int ret = 0;

	vt_initialize_header_buffer(&header);
	strcpy(header.path, path);

	if (NULL == cfg->test_name)
		strcpy(header.test_name, DEFAULT_TEST_NAME);
	else
		strcpy(header.test_name, cfg->test_name);

	if(NULL == cfg->output_file)
		strcpy(filename, vt_default_log_file_name);
	else
		strcpy(filename, cfg->output_file);

	printf("Log file: %s\n", filename);
	header.time_stamp = time(NULL);

	ret = nvme_identify_ctrl(fd, &header.raw_ctrl);
	if (ret) {
		printf("Cannot read identify device\n");
		return -1;
	}

	ret = nvme_fw_log(fd, &header.raw_fw);
	if (ret) {
		printf("Cannot read device firmware log\n");
		return -1;
	}

	vt_process_string(header.raw_ctrl.sn, sizeof(header.raw_ctrl.sn));
	vt_process_string(header.raw_ctrl.mn, sizeof(header.raw_ctrl.mn));

	ret = vt_append_header(&header, filename);
	return (ret);
}

static void vt_build_identify_lv2(unsigned int data, unsigned int start,
				  unsigned int count, const char **table,
				  bool isEnd)
{
	unsigned int i, end, pos, sh = 1;
	unsigned int temp;

	end = start + count;

	for (i = start; i < end; i++) {
		temp = ((data & (sh << i)) >> i);
		pos = i * 2;
		printf("        \"bit %u\":\"%ub  %s\"\n", i, temp, table[pos]);
		printf("                     %s", table[pos + 1]);

		if((end - 1) != i || !isEnd)
			printf(",\n");
		else
			printf("\n");
	}

	if(isEnd)
		printf("    },\n");
}

static void vt_build_power_state_descriptor(const struct nvme_id_ctrl *ctrl)
{
	unsigned int i;
	unsigned char *buf;

	printf("{\n");
	printf("\"Power State Descriptors\":{\n");
	printf("    \"NOPS\":\"Non-Operational State,\"\n");
	printf("    \"MPS\":\"Max Power Scale (0: in 0.01 Watts; 1: in 0.0001 Watts),\"\n");
	printf("    \"ENLAT\":\"Entry Latency in microseconds,\"\n");
	printf("    \"RWL\":\"Relative Write Latency,\"\n");
	printf("    \"RRL\":\"Relative Read Latency,\"\n");
	printf("    \"IPS\":\"Idle Power Scale (00b: Not reported; 01b: 0.0001 W; 10b: 0.01 W; 11b: Reserved),\"\n");
	printf("    \"APS\":\"Active Power Scale (00b: Not reported; 01b: 0.0001 W; 10b: 0.01 W; 11b: Reserved),\"\n");
	printf("    \"ACTP\":\"Active Power,\"\n");
	printf("    \"MP\":\"Maximum Power,\"\n");
	printf("    \"EXLAT\":\"Exit Latency in microsecond,\"\n");
	printf("    \"RWT\":\"Relative Write Throughput,\"\n");
	printf("    \"RRT\":\"Relative Read Throughput,\"\n");
	printf("    \"IDLP\":\"Idle Power,\"\n");
	printf("    \"APW\":\"Active Power Workload,\"\n");
	printf("    \"Ofs\":\"BYTE Offset,\"\n");

	printf("    \"Power State Descriptors\":\"\n");

	printf("%6s%10s%5s%4s%6s%10s%10s%10s%4s%4s%4s%4s%10s%4s%6s%10s%4s%5s%6s\n", "Entry", "0fs 00-03", "NOPS", "MPS", "MP", "ENLAT", "EXLAT", "0fs 12-15",
			"RWL", "RWT", "RRL", "RRT", "0fs 16-19", "IPS", "IDLP", "0fs 20-23", "APS", "APW", "ACTP");


	printf("%6s%10s%5s%4s%6s%10s%10s%10s%4s%4s%4s%4s%10s%4s%6s%10s%4s%5s%6s\n", "=====", "=========", "====", "===", "=====", "=========", "=========",
			"=========", "===", "===", "===", "===", "=========", "===", "=====", "=========", "===", "====", "=====");

	for (i = 0; i < 32; i++) {
		char s[100];
		unsigned int temp;

		printf("%6d", i);
		buf = (unsigned char*) (&ctrl->psd[i]);
		vt_convert_data_buffer_to_hex_string(&buf[0], 4, true, s);
		printf("%9sh", s);

		temp = ctrl->psd[i].flags;
		printf("%4ub", ((unsigned char)temp & 0x02));
		printf("%3ub", ((unsigned char)temp & 0x01));
		vt_convert_data_buffer_to_hex_string(&buf[0], 2, true, s);
		printf("%5sh", s);

		vt_convert_data_buffer_to_hex_string(&buf[4], 4, true, s);
		printf("%9sh", s);
		vt_convert_data_buffer_to_hex_string(&buf[8], 4, true, s);
		printf("%9sh", s);
		vt_convert_data_buffer_to_hex_string(&buf[12], 4, true, s);
		printf("%9sh", s);
		vt_convert_data_buffer_to_hex_string(&buf[15], 1, true, s);
		printf("%3sh", s);
		vt_convert_data_buffer_to_hex_string(&buf[14], 1, true, s);
		printf("%3sh", s);
		vt_convert_data_buffer_to_hex_string(&buf[13], 1, true, s);
		printf("%3sh", s);
		vt_convert_data_buffer_to_hex_string(&buf[12], 1, true, s);
		printf("%3sh", s);
		vt_convert_data_buffer_to_hex_string(&buf[16], 4, true, s);
		printf("%9sh", s);

		temp = ctrl->psd[i].idle_scale;
		snprintf(s, sizeof(s), "%u%u", (((unsigned char)temp >> 6) & 0x01), (((unsigned char)temp >> 7) & 0x01));
		printf("%3sb", s);

		vt_convert_data_buffer_to_hex_string(&buf[16], 2, true, s);
		printf("%5sh", s);
		vt_convert_data_buffer_to_hex_string(&buf[20], 4, true, s);
		printf("%9sh", s);

		temp = ctrl->psd[i].active_work_scale;
		snprintf(s, sizeof(s), "%u%u", (((unsigned char)temp >> 6) & 0x01), (((unsigned char)temp >> 7) & 0x01));
		printf("%3sb", s);
		snprintf(s, sizeof(s), "%u%u%u", (((unsigned char)temp) & 0x01), (((unsigned char)temp >> 1) & 0x01), (((unsigned char)temp >> 2) & 0x01));
		printf("%4sb", s);

		vt_convert_data_buffer_to_hex_string(&buf[20], 2, true, s);
		printf("%5sh", s);
		printf("\n");
	}

	printf("    \"}\n}\n");

}

static void vt_dump_hex_data(const unsigned char *pbuff, size_t pbuffsize) {

	char textbuf[33];
	unsigned long int i, j;

	textbuf[32] = '\0';
	printf("[%08X] ", 0);
	for (i = 0; i < pbuffsize; i++) {
		printf("%02X ", pbuff[i]);

		if (pbuff[i] >= ' ' && pbuff[i] <= '~') 
			textbuf[i % 32] = pbuff[i];
		else 
			textbuf[i % 32] = '.';

		if ((((i + 1) % 8) == 0) || ((i + 1) == pbuffsize)) {
			printf(" ");
			if ((i + 1) % 32 == 0) {
				printf(" %s\n", textbuf);
				if((i + 1) != pbuffsize)
				    printf("[%08lX] ", (i + 1));
			} 
			else if (i + 1 == pbuffsize) {
				textbuf[(i + 1) % 32] = '\0';
				if(((i + 1) % 8) == 0)
					printf(" ");

				for (j = ((i + 1) % 32); j < 32; j++) {
					printf("   ");
					if(((j + 1) % 8) == 0)
						printf(" ");
				}

				printf("%s\n", textbuf);
			}
		}
	}
}

static void vt_parse_detail_identify(const struct nvme_id_ctrl *ctrl)
{
	unsigned char *buf;
	unsigned int temp, pos;
	char s[1024] = "";

	const char *CMICtable[6] = {"0 = the NVM subsystem contains only a single NVM subsystem port",
								"1 = the NVM subsystem may contain more than one subsystem ports",
								"0 = the NVM subsystem contains only a single controller",
								"1 = the NVM subsystem may contain two or more controllers (see section 1.4.1)",
								"0 = the controller is associated with a PCI Function or a Fabrics connection",
								"1 = the controller is associated with an SR-IOV Virtual Function"};

	const char *OAEStable[20] = {"Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "0 = does not support sending the Namespace Attribute Notices event nor the associated Changed Namespace List log page",
								 "1 = supports sending the Namespace Attribute Notices  & the associated Changed Namespace List log page",
								 "0 = does not support sending Firmware Activation Notices event",
								 "1 = supports sending Firmware Activation Notices"};

	const char *CTRATTtable[4] = {"0 = does not support a 128-bit Host Identifier",
								  "1 = supports a 128-bit Host Identifier",
								  "0 = does not support Non-Operational Power State Permissive Mode",
								  "1 = supports Non-Operational Power State Permissive Mode"};

	const char *OACStable[18] = {"0 = does not support the Security Send and Security Receive commands",
								 "1 = supports the Security Send and Security Receive commands",
								 "0 = does not support the Format NVM command",
								 "1 = supports the Format NVM command",
								 "0 = does not support the Firmware Commit and Firmware Image Download commands",
								 "1 = supports the Firmware Commit and Firmware Image Download commands",
								 "0 = does not support the Namespace Management capability",
								 "1 = supports the Namespace Management capability",
								 "0 = does not support the Device Self-test command",
								 "1 = supports the Device Self-test command",
								 "0 = does not support Directives",
								 "1 = supports Directive Send & Directive Receive commands",
								 "0 = does not support the NVMe-MI Send and NVMe-MI Receive commands",
								 "1 = supports the NVMe-MI Send and NVMe-MI Receive commands",
								 "0 = does not support the Virtualization Management command",
								 "1 = supports the Virtualization Management command",
								 "0 = does not support the Doorbell Buffer Config command",
								 "1 = supports the Doorbell Buffer Config command"};

	const char *FRMWtable[10] = {"0 = the 1st firmware slot (slot 1) is read/write",
								 "1 = the 1st firmware slot (slot 1) is read only",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "Reserved",
								 "0 = requires a reset for firmware to be activated",
								 "1 = supports firmware activation without a reset"};

	const char *LPAtable[8] = {"0 = does not support the SMART / Health information log page on a per namespace basis",
							   "1 = supports the SMART / Health information log page on a per namespace basis",
							   "0 = does not support the Commands Supported & Effects log page",
							   "1 = supports the Commands Supported Effects log page",
							   "0 = does not support extended data for Get Log Page",
							   "1 = supports extended data for Get Log Page (including extended Number of Dwords and Log Page Offset fields)",
							   "0 = does not support the Telemetry Host-Initiated and Telemetry Controller-Initiated log pages and Telemetry Log Notices events",
							   "1 = supports the Telemetry Host-Initiated and Telemetry Controller-Initiated log pages and sending Telemetry Log Notices"							   };

	const char *AVSCCtable[2] = {"0 = the format of all Admin Vendor Specific Commands are vendor specific",
								 "1 = all Admin Vendor Specific Commands use the format defined in NVM Express specification"};

	const char *APSTAtable[2] = {"0 = does not support autonomous power state transitions",
								 "1 = supports autonomous power state transitions"};

	const char *DSTOtable[2] =  {"0 = the NVM subsystem supports one device self-test operation per controller at a time",
								 "1 = the NVM subsystem supports only one device self-test operation in progress at a time"};

	const char *HCTMAtable[2] = {"0 = does not support host controlled thermal management",
								 "1 = supports host controlled thermal management. Supports Set Features & Get Features commands with the Feature Identifier field set to 10h"};

	const char *SANICAPtable[6] =  {"0 = does not support the Crypto Erase sanitize operation",
									"1 = supports the Crypto Erase sanitize operation",
									"0 = does not support the Block Erase sanitize operation",
									"1 = supports the Block Erase sanitize operation",
									"0 = does not support the Overwrite sanitize operation",
									"1 = supports the Overwrite sanitize operation"};

	const char *ONCStable[14] =  {"0 = does not support the Compare command",
								  "1 = supports the Compare command",
								  "0 = does not support the Write Uncorrectable command",
								  "1 = supports the Write Uncorrectable command",
								  "0 = does not support the Dataset Management command",
								  "1 = supports the Dataset Management command",
								  "0 = does not support the Write Zeroes command",
								  "1 = supports the Write Zeroes command",
								  "0 = does not support the Save field set to a non-zero value in the Set Features and the Get Features commands",
								  "1 = supports the Save field set to a non-zero value in the Set Features and the Get Features commands", \
								  "0 = does not support reservations",
								  "1 = supports reservations",
								  "0 = does not support the Timestamp feature (refer to section 5.21.1.14)",
								  "1 = supports the Timestamp feature"};

	const char *FUSEStable[2] = {"0 =  does not support the Compare and Write fused operation",
								 "1 =  supports the Compare and Write fused operation"};

	const char *FNAtable[6] = {"0 = supports format on a per namespace basis",
							   "1 = all namespaces shall be configured with the same attributes and a format (excluding secure erase) of any namespace results in a format of all namespaces in an NVM subsystem",
							   "0 = any secure erase performed as part of a format results in a secure erase of a particular namespace specified",
							   "1 = any secure erase performed as part of a format operation results in a secure erase of all namespaces in the NVM subsystem",
							   "0 = cryptographic erase is not supported",
							   "1 = cryptographic erase is supported as part of the secure erase functionality"};

	const char *VWCtable[2] = {"0 = a volatile write cache is not present",
							   "1 = a volatile write cache is present"};

	const char *ICSVSCCtable[2] = {"0 = the format of all NVM Vendor Specific Commands are vendor specific",
								 "1 = all NVM Vendor Specific Commands use the format defined in NVM Express specification"};

	const char *SGLSSubtable[4] =  {"00b = SGLs are not supported",
									"01b = SGLs are supported. There is no alignment nor granularity requirement for Data Blocks",
									"10b = SGLs are supported. There is a Dword alignment and granularity requirement for Data Blocks",
									"11b = Reserved"};

	const char *SGLStable[42] =  {"Used",
								  "Used",
								  "Used",
								  "Used",
								  "0 = does not support the Keyed SGL Data Block descriptor",
								  "1 = supports the Keyed SGL Data Block descriptor",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "Reserved",
								  "0 = the SGL Bit Bucket descriptor is not supported",
								  "1 = the SGL Bit Bucket descriptor is supported",
								  "0 = use of a byte aligned contiguous physical buffer of metadata is not supported",
								  "1 = use of a byte aligned contiguous physical buffer of metadata is supported",
								  "0 = the SGL length shall be equal to the amount of data to be transferred",
								  "1 = supports commands that contain a data or metadata SGL of a length larger than the amount of data to be transferred",
								  "0 = use of Metadata Pointer (MPTR) that contains an address of an SGL segment containing exactly one SGL Descriptor that is Qword aligned is not supported",
								  "1 = use of Metadata Pointer (MPTR) that contains an address of an SGL segment containing exactly one SGL Descriptor that is Qword aligned is supported",
								  "0 = the Address field specifying an offset is not supported",
	                              "1 = supports the Address field in SGL Data Block, SGL Segment, and SGL Last Segment descriptor types specifying an offset"};

	buf = (unsigned char *)(ctrl);

	printf("{\n");
	vt_convert_data_buffer_to_hex_string(buf, 2, true, s);
	printf("    \"PCI Vendor ID\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[2], 2, true, s);
	printf("    \"PCI Subsystem Vendor ID\":\"%sh\",\n",  s);
	printf("    \"Serial Number\":\"%s\",\n", ctrl->sn);
	printf("    \"Model Number\":\"%s\",\n", ctrl->mn);
	printf("    \"Firmware Revision\":\"%-.*s\",\n", (int)sizeof(ctrl->fr), ctrl->fr);
	vt_convert_data_buffer_to_hex_string(&buf[72], 1, true, s);
	printf("    \"Recommended Arbitration Burst\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[73], 3, true, s);
	printf("    \"IEEE OUI Identifier\":\"%sh\",\n", s);

	temp = ctrl->cmic;
	printf("    \"Controller Multi-Path I/O and Namespace Sharing Capabilities\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[76], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 3, CMICtable, true);

	vt_convert_data_buffer_to_hex_string(&buf[77], 1, true, s);
	printf("    \"Maximum Data Transfer Size\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[78], 2, true, s);
	printf("    \"Controller ID\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[80], 4, true, s);
	printf("    \"Version\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[84], 4, true, s);
	printf("    \"RTD3 Resume Latency\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[88], 4, true, s);
	printf("    \"RTD3 Entry Latency\":\"%sh\",\n", s);

	temp = le32_to_cpu(ctrl->oaes);
	printf("    \"Optional Asynchronous Events Supported\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[92], 4, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 8, 2, OAEStable, true);

	temp = le32_to_cpu(ctrl->ctratt);
	printf("    \"Controller Attributes\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[96], 4, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 2, CTRATTtable, true);

	vt_convert_data_buffer_to_hex_string(&buf[122], 16, true, s);
	printf("    \"FRU Globally Unique Identifier\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[240], 16, true, s);
	printf("    \"NVMe Management Interface Specification\":\"%sh\",\n", s);

	temp = le16_to_cpu(ctrl->oacs);
	printf("    \"Optional Admin Command Support\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[256], 2, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 9, OACStable, true);

	vt_convert_data_buffer_to_hex_string(&buf[258], 1, true, s);
	printf("    \"Abort Command Limit\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[259], 1, true, s);
	printf("    \"Asynchronous Event Request Limit\":\"%sh\",\n", s);

	temp = ctrl->frmw;
	printf("    \"Firmware Updates\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[260], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 1, FRMWtable, false);
	vt_convert_data_buffer_to_hex_string(&buf[260], 1, true, s);
	printf("        \"Firmware Slot\":\"%uh\",\n", ((ctrl->frmw >> 1) & 0x07));
	vt_build_identify_lv2(temp, 4, 1, FRMWtable, true);

	temp = ctrl->lpa;
	printf("    \"Log Page Attributes\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[261], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 4, LPAtable, true);

	vt_convert_data_buffer_to_hex_string(&buf[262], 1, true, s);
	printf("    \"Error Log Page Entries\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[263], 1, true, s);
	printf("    \"Number of Power States Support\":\"%sh\",\n", s);

	temp = ctrl->avscc;
	printf("    \"Admin Vendor Specific Command Configuration\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[264], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 1, AVSCCtable, true);

	temp = ctrl->apsta;
	printf("    \"Autonomous Power State Transition Attributes\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[265], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 1, APSTAtable, true);

	vt_convert_data_buffer_to_hex_string(&buf[266], 2, true, s);
	printf("    \"Warning Composite Temperature Threshold\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[268], 2, true, s);
	printf("    \"Critical Composite Temperature Threshold\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[270], 2, true, s);
	printf("    \"Maximum Time for Firmware Activation\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[272], 4, true, s);
	printf("    \"Host Memory Buffer Preferred Size\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[276], 4, true, s);
	printf("    \"Host Memory Buffer Minimum Size\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[280], 16, true, s);
	printf("    \"Total NVM Capacity\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[296], 16, true, s);
	printf("    \"Unallocated NVM Capacity\":\"%sh\",\n", s);

	temp = le32_to_cpu(ctrl->rpmbs); 
	printf("    \"Replay Protected Memory Block Support\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[312], 4, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	printf("        \"Number of RPMB Units\":\"%u\",\n", (temp & 0x00000003));
	snprintf(s, sizeof(s), ((temp >> 3) & 0x00000007)? "Reserved" : "HMAC SHA-256");
	printf("        \"Authentication Method\":\"%u: %s\",\n", ((temp >> 3) & 0x00000007), s);
	printf("        \"Total Size\":\"%u\",\n", ((temp >> 16) & 0x000000FF));
	printf("        \"Access Size\":\"%u\",\n", ((temp >> 24) & 0x000000FF));
	printf("    },\n");

	vt_convert_data_buffer_to_hex_string(&buf[316], 2, true, s);
	printf("    \"Extended Device Self-test Time\":\"%sh\",\n", s);

	temp = ctrl->dsto;
	printf("    \"Device Self-test Options\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[318], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 1, DSTOtable, true);

	vt_convert_data_buffer_to_hex_string(&buf[319], 1, true, s);
	printf("    \"Firmware Update Granularity\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[320], 1, true, s);
	printf("    \"Keep Alive Support\":\"%sh\",\n", s);

	temp = le16_to_cpu(ctrl->hctma);
	printf("    \"Host Controlled Thermal Management Attributes\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[322], 2, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 1, HCTMAtable, true);

	vt_convert_data_buffer_to_hex_string(&buf[324], 2, true, s);
	printf("    \"Minimum Thermal Management Temperature\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[326], 2, true, s);
	printf("    \"Maximum Thermal Management Temperature\":\"%sh\",\n", s);

	temp = le16_to_cpu(ctrl->sanicap);
	printf("    \"Sanitize Capabilities\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[328], 2, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 3, SANICAPtable, true);

	temp = ctrl->sqes;
	printf("    \"Submission Queue Entry Size\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[512], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	printf("        \"Maximum Size\":\"%u\",\n", (temp & 0x0000000F));
	printf("        \"Required Size\":\"%u\",\n", ((temp >> 4) & 0x0000000F));
	printf("    }\n");

	temp = ctrl->cqes;
	printf("    \"Completion Queue Entry Size\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[513], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	printf("        \"Maximum Size\":\"%u\",\n", (temp & 0x0000000F));
	printf("        \"Required Size\":\"%u\",\n", ((temp >> 4) & 0x0000000F));
	printf("    }\n");

	vt_convert_data_buffer_to_hex_string(&buf[514], 2, true, s);
	printf("    \"Maximum Outstanding Commands\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[516], 4, true, s);
	printf("    \"Number of Namespaces\":\"%sh\",\n", s);

	temp = le16_to_cpu(ctrl->oncs);
	printf("    \"Optional NVM Command Support\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[520], 2, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 7, ONCStable, true);

	temp = le16_to_cpu(ctrl->fuses);
	printf("    \"Fused Operation Support\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[522], 2, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 1, FUSEStable, true);

	temp = ctrl->fna;
	printf("    \"Format NVM Attributes\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[524], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 3, FNAtable, true);

	temp = ctrl->vwc;
	printf("    \"Volatile Write Cache\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[525], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 1, VWCtable, true);

	vt_convert_data_buffer_to_hex_string(&buf[526], 2, true, s);
	printf("    \"Atomic Write Unit Normal\":\"%sh\",\n", s);
	vt_convert_data_buffer_to_hex_string(&buf[528], 2, true, s);
	printf("    \"Atomic Write Unit Power Fail\":\"%sh\",\n", s);

	temp = ctrl->icsvscc;
	printf("    \"NVM Vendor Specific Command Configuration\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[530], 1, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	vt_build_identify_lv2(temp, 0, 1, ICSVSCCtable, true);

	vt_convert_data_buffer_to_hex_string(&buf[532], 2, true, s);
	printf("    \"Atomic Compare 0 Write Unit\":\"%sh\",\n", s);

	temp = le32_to_cpu(ctrl->sgls);
	printf("    \"SGL Support\":{\n");
	vt_convert_data_buffer_to_hex_string(&buf[536], 4, true, s);
	printf("        \"Value\":\"%sh\",\n", s);
	pos = (temp & 0x00000003);
	printf("        \"bit 1:0\":\"%s\",\n", SGLSSubtable[pos]);
	vt_build_identify_lv2(temp, 2, 1, SGLStable, false);
	vt_build_identify_lv2(temp, 16, 5, SGLStable, true);

	vt_convert_data_buffer_to_hex_string(&buf[768], 256, false, s);
	printf("    \"NVM Subsystem NVMe Qualified Name\":\"%s\",\n", s);
	printf("}\n\n");

	vt_build_power_state_descriptor(ctrl);


	printf("\n{\n");
	printf("\"Vendor Specific\":\"\n");
	vt_dump_hex_data(&buf[3072], 1024);
	printf("\"}\n");
}

static int vt_save_smart_to_vtview_log(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
	int err = 0;
	int fd, ret;
	long int total_time = 0;
	long int freq_time = 0;
	long int cur_time = 0;
	long int remain_time = 0;
	long int start_time = 0;
	long int end_time = 0;
	char path[256] = "";
	char *desc = "Save SMART data into log file with format that is easy to analyze (comma delimited). Maximum log file will be 4K.\n\n"
		"Typical usages:\n\n"
		"Temperature characterization: \n"
		"\tvirtium save-smart-to-vtview-log /dev/yourDevice --run-time=100 --record-frequency=0.25 --test-name=burn-in-at-(-40)\n\n"
		"Endurance testing : \n"
		"\tvirtium save-smart-to-vtview-log /dev/yourDevice --run-time=100 --record-frequency=1 --test-name=Endurance-test-JEDEG-219-workload\n\n"
		"Just logging :\n"
		"\tvirtium save-smart-to-vtview-log /dev/yourDevice";

	const char *run_time = "(optional) Number of hours to log data (default = 20 hours)";
	const char *freq = "(optional) How often you want to log SMART data (0.25 = 15' , 0.5 = 30' , 1 = 1 hour, 2 = 2 hours, etc.). Default = 10 hours.";
	const char *output_file = "(optional) Name of the log file (give it a name that easy for you to remember what the test is). You can leave it blank too, we will take care it for you.";
	const char *test_name = "(optional) Name of the test you are doing. We use this as part of the name of the log file.";

	struct vtview_save_log_settings cfg = {
		.run_time_hrs = 20,
		.log_record_frequency_hrs = 10,
		.output_file = NULL,
		.test_name = NULL,
	};

	OPT_ARGS(opts) = {
		OPT_DOUBLE("run-time",  'r', &cfg.run_time_hrs,             run_time),
		OPT_DOUBLE("freq",      'f', &cfg.log_record_frequency_hrs, freq),
		OPT_FILE("output-file", 'o', &cfg.output_file,              output_file),
		OPT_STRING("test-name", 'n', "NAME", &cfg.test_name,        test_name),
		OPT_END()
	};

	vt_generate_vtview_log_file_name(vt_default_log_file_name);

	if (argc >= 2)
		strcpy(path, argv[1]);

	fd = parse_and_open(argc, argv, desc, opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return (fd);
	}

	printf("Running...\n");
	printf("Collecting data for device %s\n", path);
	printf("Running for %lf hour(s)\n", cfg.run_time_hrs);
	printf("Logging SMART data for every %lf hour(s)\n", cfg.log_record_frequency_hrs);

	ret = vt_update_vtview_log_header(fd, path, &cfg);
	if (ret) {
		err = EINVAL;
		close(fd);
		return (err);
	}

	total_time = cfg.run_time_hrs * (float)HOUR_IN_SECONDS;
	freq_time = cfg.log_record_frequency_hrs * (float)HOUR_IN_SECONDS;

	if(freq_time == 0)
		freq_time = 1;

	start_time = time(NULL);
	end_time = start_time + total_time;

	fflush(stdout);

	while (1) {
		cur_time = time(NULL);
		if(cur_time >= end_time)
			break;

		ret = vt_add_entry_to_log(fd, path, &cfg);
		if (ret) {
			printf("Cannot update driver log\n");
			break;
		}

		remain_time = end_time - cur_time;
		freq_time = MIN2(freq_time, remain_time);
		sleep(freq_time);
		fflush(stdout);
	}

	close (fd);
	return (err);
}

static int vt_show_identify(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
	int err = 0;
	int fd ,ret;
	struct nvme_id_ctrl ctrl;
	char *desc = "Parse identify data to json format\n\n"
		"Typical usages:\n\n"
		"virtium show-identify /dev/yourDevice\n";

	OPT_ARGS(opts) = {
		OPT_END()
	};

	fd = parse_and_open(argc, argv, desc, opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return (fd);
	}

	ret = nvme_identify_ctrl(fd, &ctrl);
	if (ret) {
		printf("Cannot read identify device\n");
		close (fd);
		return (-1);
	}

	vt_process_string(ctrl.sn, sizeof(ctrl.sn));
	vt_process_string(ctrl.mn, sizeof(ctrl.mn));
	vt_parse_detail_identify(&ctrl);

	close(fd);
	return (err);
}

static int vt_get_series32_fw_string(int argc, char **argv, struct command *cmd, struct plugin *plugin) 
{
	int fd, ret;
	int cmd_data_len = 512;
	int ret_len = 100;

	OPT_ARGS(opts) = {
		OPT_END()
	};

	fd = parse_and_open(argc, argv, "", opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return -1;
	}
	
	__u8* data = calloc(cmd_data_len, sizeof(__u8));
	data[0] = 0x16;

	struct nvme_admin_cmd setup_vsc_get_fw = {
		.opcode = 0xfd,
		.cdw12 = 0x00fc,
		.cdw10 = 128,
		.data_len = cmd_data_len,
		.addr = (uintptr_t)data
	};

	// setup vsc
	ret = nvme_submit_admin_passthru(fd, &setup_vsc_get_fw);
	if (ret != 0) {
		printf("Error during admin cmd passthru (ret = %d)!\n", ret);
		free(data);
		return ret;
	}

	struct nvme_admin_cmd vsc_get_fw = {
		.opcode = 0xfe,
		.cdw12 = 0x00fd,
		.cdw10 = 25,
		.data_len = ret_len,
		.addr = (uintptr_t)data
	};

	// exec vsc and get fw string back
	ret = nvme_submit_admin_passthru(fd, &vsc_get_fw);
	if (ret != 0) {
		printf("Error during admin cmd passthru (ret = %d)!\n", ret);
		free(data);
		return ret;
	}
	printf("F/W Version String: %s\n", data);
	
	free(data);

	return ret;
}

static int vt_get_sn_info(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
	int fd, ret;
	int cmd_data_len = 512;
	int ret_len = 20;
    
	OPT_ARGS(opts) = {
		OPT_END()
	};
    
	fd = parse_and_open(argc, argv, "", opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return -1;
	}
    
	__u8* data = calloc(cmd_data_len, sizeof(__u8));
	data[0] = 0x25;
	data[1] = 0x05;
	data[2] = 0x01;

	struct nvme_admin_cmd setup_vsc_get_fw = {
		.opcode = 0xfd,
		.cdw12 = 0x00fc,
		.cdw10 = 128,
		.data_len = cmd_data_len,
		.addr = (uintptr_t)data
	};

	// setup vsc
	ret = nvme_submit_admin_passthru(fd, &setup_vsc_get_fw);
	if (ret != 0) {
		printf("Error during admin cmd passthru setup (ret = %d)!\n", ret);
		free(data);
		return ret;
	}

	struct nvme_admin_cmd vsc_get_sn = {
		.opcode = 0xfe,
		.cdw12 = 0x00fd,
		.cdw10 = 5,
		.data_len = ret_len,
		.addr = (uintptr_t)data
	};

	// exec vsc and get fw string back
	ret = nvme_submit_admin_passthru(fd, &vsc_get_sn);
	if (ret != 0) {
		printf("Error during admin cmd passthru (ret = %d)!\n", ret);
		free(data);
		return ret;
	}
	printf("SN String: %s\n", data);
    
	free(data);

	return ret;
}

static int vt_get_mn_info(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
	int fd, ret;
	int cmd_data_len = 512;
	int ret_len = 40;
    
	OPT_ARGS(opts) = {
		OPT_END()
	};
    
	fd = parse_and_open(argc, argv, "", opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return -1;
	}
    
	__u8* data = calloc(cmd_data_len, sizeof(__u8));
	data[0] = 0x25;
	data[1] = 0x05;

	struct nvme_admin_cmd setup_vsc_get_fw = {
		.opcode = 0xfd,
		.cdw12 = 0x00fc,
		.cdw10 = 128,
		.data_len = cmd_data_len,
		.addr = (uintptr_t)data
	};

	// setup vsc
	ret = nvme_submit_admin_passthru(fd, &setup_vsc_get_fw);
	if (ret != 0) {
		printf("Error during admin cmd passthru setup (ret = %d)!\n", ret);
		free(data);
		return ret;
	}

	struct nvme_admin_cmd vsc_get_mn = {
		.opcode = 0xfe,
		.cdw12 = 0x00fd,
		.cdw10 = 10,
		.data_len = ret_len,
		.addr = (uintptr_t)data
	};

	// exec vsc and get mn string back
	ret = nvme_submit_admin_passthru(fd, &vsc_get_mn);
	if (ret != 0) {
		printf("Error during admin cmd passthru (ret = %d)!\n", ret);
		free(data);
		return ret;
	}
	printf("MN String: %s\n", data);
    
	free(data);

	return ret;
}

static int vt_get_event_log(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
	int fd, ret;
	int cmd_data_len = 512;
	int ret_len = 12;
 
	OPT_ARGS(opts) = {
		OPT_END()
	};
 
	fd = parse_and_open(argc, argv, "", opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return -1;
	}
 
	__u8* data = calloc(cmd_data_len, sizeof(__u8));
	data[0] = 0x82;
	data[1] = 1;
	data[2] = 0;
	data[3] = 0;
	data[4] = 2;
 
	struct nvme_admin_cmd setup_vsc_get_info = {
		.opcode = 0xfd,
		.cdw12 = 0x00fc,
		.cdw10 = 128,
		.data_len = cmd_data_len,
		.addr = (uintptr_t)data
	};
 
	// setup vsc to get event info
	ret = nvme_submit_admin_passthru(fd, &setup_vsc_get_info);
	if (ret != 0) {
		printf("Error during admin cmd passthru setup (ret = %d)!\n", ret);
		free(data);
		return ret;
	} 
 
	struct nvme_admin_cmd vsc_get_info = {
		.opcode = 0xfe,
		.cdw12 = 0x00fd,
		.cdw10 = 3,
		.data_len = ret_len,
		.addr = (uintptr_t)data
	};
 
	// exec vsc and get event info
	ret = nvme_submit_admin_passthru(fd, &vsc_get_info);
	if (ret != 0) {
		printf("Error during admin cmd passthru (ret = %d)!\n", ret);
		free(data);
		return ret;
	}
   
	// convert event info into a readable format using readinfo.py
	FILE *fp;
	fp = fopen("_temp.dat","wb");
	for(int i =0; i<= ret_len;i++)
	{
		fputc(data[i],fp);
	}
	fclose(fp);
	char str[80];
	fp = popen("python3 readinfo.py _temp.dat","r");
	while(fgets(str,sizeof(str),fp) != NULL)
	{
		NULL;
	}
	pclose(fp);
	remove("_temp.dat");
   
	int numRecords = atoi(strtok(str," "));
	int recordSize = atoi(strtok(NULL," "));
	int recordType = atoi(strtok(NULL," "));
 
	if (recordType != 2)
	{
		printf("Expected token type logs. Exiting\n");
		return ret;
		free(data);
	}
   
	// create vsc to get event logs
	__u8* dataInput = calloc(cmd_data_len, sizeof(__u8));
	dataInput[0] = 0x83;
	dataInput[1] = 0x01;
   
	struct nvme_admin_cmd setup_vsc_get_log = {
		.opcode = 0xfd,
		.cdw12 = 0x00fc,
		.cdw10 = 128,
		.data_len = cmd_data_len,
		.addr = (uintptr_t)dataInput
	};

	// exec vsc to get event logs
	printf("Extracting %d event log blocks from drive to eventlog.dat\n",numRecords);
	fp = fopen("eventlog.dat","wb");
	for (int i=0; i<numRecords;i++)
	{
		__u8* dataOutput = calloc(recordSize, sizeof(__u8));
		ret = nvme_submit_admin_passthru(fd, &setup_vsc_get_log);
		if (ret != 0) {
			printf("Error during admin cmd passthru setup(ret = %d)!\n", ret);
			free(data);
			free(dataInput);
			free(dataOutput);
			fclose(fp);
			return ret;
		}
            
		struct nvme_admin_cmd vsc_get_log = {
			.opcode = 0xfe,
			.cdw12 = 0x00fd,
			.cdw10 = recordSize,
			.data_len = recordSize,
			.addr = (uintptr_t)dataOutput
		};
       
		ret = nvme_submit_admin_passthru(fd, &vsc_get_log);
		if (ret != 0) {
			printf("Error during admin cmd passthru(ret = %d)!\n", ret);
			free(data);
			free(dataInput);
			free(dataOutput);
			fclose(fp);
			return ret;
		}
		for(int i =0; i<recordSize;i++)
		{
			fputc(dataOutput[i],fp);
		}
		free(dataOutput);
	}
	printf("Finished extracting event log blocks\n");
	fclose(fp);
	free(data);
	free(dataInput);
	return ret;
}

static int vt_get_crash_info(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
 	int fd, ret;
	int cmd_data_len = 512;
	int ret_len = 4096;
	bool do_erase = false;
	const char *erase = "(optional) Erase the crash info";
 
	OPT_ARGS(opts) = {
		OPT_FLAG("do-erase",'e',&do_erase,erase),
		OPT_END()
	};
 
	fd = parse_and_open(argc, argv, "", opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return -1;
	}
 
	__u8* data = calloc(cmd_data_len, sizeof(__u8));
	data[0] = 0x77;
	data[1] = 0x01;
 
	struct nvme_admin_cmd setup_vsc_get_info = {
		.opcode = 0xfd,
		.cdw12 = 0x00fc,
		.cdw10 = 128,
		.data_len = cmd_data_len,
		.addr = (uintptr_t)data
	};
 
	// setup vsc to get crash info
	ret = nvme_submit_admin_passthru(fd, &setup_vsc_get_info);
	if (ret != 0) {
		printf("Error during admin cmd passthru setup (ret = %d)!\n", ret);
		free(data);
		return ret;
	} 
	__u8* dataOutput = calloc(ret_len, sizeof(__u8));
	struct nvme_admin_cmd vsc_get_info = {
		.opcode = 0xfe,
		.cdw12 = 0x00fd,
		.cdw10 = 1024,
		.data_len = ret_len,
		.addr = (uintptr_t)dataOutput
	};
 
	// exec vsc and get crash info
	ret = nvme_submit_admin_passthru(fd, &vsc_get_info);
	if (ret != 0) {
		printf("Error during admin cmd passthru (ret = %d)!\n", ret);
		free(dataOutput);
		free(data);
		return ret;
	}
   
	// convert crash info into a readable format using readcrashinfo.py
	FILE *fp;
	fp = fopen("_temp.dat","wb");
	for(int i =0; i<= ret_len;i++)
	{
		fputc(dataOutput[i],fp);
	}
	fclose(fp);
	char str[80];
	fp = popen("python3 readcrashinfo.py _temp.dat","r");
	while(fgets(str,sizeof(str),fp) != NULL)
	{
		NULL;
	}
	pclose(fp);
	remove("_temp.dat");
   
	int pageSize = atoi(strtok(str," "));
	int pageCount = atoi(strtok(NULL," "));
	int numImages = atoi(strtok(NULL," "));
	int imageSize = atoi(strtok(NULL," "));
	printf("Number of images: %d\nPage Size: %d\nPage Count: %d\n"
	       "Image Size: %d\n",numImages,pageSize,pageCount,imageSize);

	if (do_erase)
	{
		if (numImages != 0)
		{
			__u8* dataInput = calloc(cmd_data_len, sizeof(__u8));
			dataInput[0] = 0x79;
			dataInput[1] = 0x01;
			struct nvme_admin_cmd setup_vsc_delete_info = {
				.opcode = 0xfd,
				.cdw12 = 0x00fc,
				.cdw10 = 128,
				.data_len = cmd_data_len,
				.addr = (uintptr_t)dataInput
			};
			ret = nvme_submit_admin_passthru(fd, &setup_vsc_delete_info);
			if (ret != 0) 
			{
				printf("Error during admin cmd passthru setup, erase crashdump failed (ret = %d)!\n", ret);
				free(dataInput);
				free(dataOutput);
				free(data);
				return ret;
			}
			struct nvme_admin_cmd vsc_delete_info = {
				.opcode = 0xfe,
				.cdw12 = 0x00fd,
				.cdw10 = 1024,
				.data_len = cmd_data_len,
				.addr = (uintptr_t)dataOutput
			};
			ret = nvme_submit_admin_passthru(fd, &vsc_delete_info);
			if (ret != 0) 
			{
				printf("Error during admin cmd passthru, erase crashdump failed (ret = %d)!\n", ret);
				free(dataInput);
				free(dataOutput);
				free(data);
				return ret;
			}
			else
			{
				printf("Crashdump erased\n");
			}
			free(dataInput);
		}
		else
		{
			printf("Nothing to erase\n");
		}
	}
	free(dataOutput);
	free(data);
	return ret;
}

static int vt_get_crash_dump(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
   	int fd, ret;
   	int cmd_data_len = 512;
   	int ret_len = 4096;
   	int image_num = 1;
   	const char *img_num = "(optional) Set the dump image";
   	OPT_ARGS(opts) = {
       	OPT_INT("img-dump",'i',&image_num,img_num),
       	OPT_END()
   	};
   	fd = parse_and_open(argc, argv, "", opts);
   	if (fd < 0) {
       	printf("Error parse and open (fd = %d)\n", fd);
       	return -1;
   	}

   	__u8* data = calloc(cmd_data_len, sizeof(__u8));
   	data[0] = 0x77;
   	data[1] = 0x01;
   	struct nvme_admin_cmd setup_vsc_get_info = {
       	.opcode = 0xfd,
       	.cdw12 = 0x00fc,
       	.cdw10 = 128,
       	.data_len = cmd_data_len,
       	.addr = (uintptr_t)data
   	};
   	// setup vsc to get crash info
   	ret = nvme_submit_admin_passthru(fd, &setup_vsc_get_info);
   	if (ret != 0) {
       	printf("Error during admin cmd passthru setup (ret = %d)!\n", ret);
       	free(data);
       	return ret;
   	}
   	__u8* dataOutput = calloc(ret_len, sizeof(__u8));
   	struct nvme_admin_cmd vsc_get_info = {
       	.opcode = 0xfe,
       	.cdw12 = 0x00fd,
       	.cdw10 = 1024,
       	.data_len = ret_len,
       	.addr = (uintptr_t)dataOutput
   	};
   	// exec vsc and get crash info
   	ret = nvme_submit_admin_passthru(fd, &vsc_get_info);
   	if (ret != 0) {
       	printf("Error during admin cmd passthru (ret = %d)!\n", ret);
       	free(dataOutput);
       	free(data);
       	return ret;
   	}
 
   	// convert crash info into a readable format using readcrashinfo.py
   	FILE *fp;
   	fp = fopen("_temp.dat","wb");
   	for(int i =0; i<= ret_len;i++)
   	{
       	fputc(dataOutput[i],fp);
   	}
   	fclose(fp);
   	char str[80];
   	fp = popen("python3 readcrashinfo.py _temp.dat","r");
   	while(fgets(str,sizeof(str),fp) != NULL)
   	{
       	NULL;
   	}
   	pclose(fp);
   	remove("_temp.dat");
 
   	int pageSize = atoi(strtok(str," "));
   	pageSize = pageSize + 0x200;
   	int pageWords = pageSize / 4;
   	int pageCount = atoi(strtok(NULL," "));
   	int numImages = atoi(strtok(NULL," "));
	//int imageSize = atoi(strtok(NULL," "));
	if (image_num > numImages)
	{
    	printf("No crash dump %d on drive. Only %d images. Exiting\n",image_num,numImages);
    	free(data);
    	free(dataOutput);
    	return ret;
	}
 
   	// create vsc file to get crash data
	__u8* dataInput = calloc(cmd_data_len, sizeof(__u8));
	dataInput[0] = 0x78;
	dataInput[1] = 0x01;
	// patch in image number
	dataInput[2] = image_num;
	printf("Extracting %d pages of crash data from drive to file crashdump.dat\n",pageCount);
	int index = 3;
	fp = fopen("crashdump.dat","wb");
	for (int i=0; i< pageCount;i++)
	{

		//patch in page number
	   	dataInput[index] = i%256;
	   	dataInput[index+1] = i/256;
	   	index = index +2;
	   	struct nvme_admin_cmd setup_vsc_get_dump = {
    		.opcode = 0xfd,
       		.cdw12 = 0x00fc,
       		.cdw10 = 128,
       		.data_len = cmd_data_len,
       		.addr = (uintptr_t)dataInput
   		};
		ret = nvme_submit_admin_passthru(fd, &setup_vsc_get_dump);
   		if (ret != 0) {
       		printf("Error during admin cmd passthru setup (ret = %d)!\n", ret);
       		free(data);
			free(dataInput);
       		return ret;
   		}
		__u8* crashData = calloc(pageSize, sizeof(__u8));
		struct nvme_admin_cmd vsc_get_dump = {
    		.opcode = 0xfe,
       		.cdw12 = 0x00fd,
       		.cdw10 = pageWords,
       		.data_len = cmd_data_len,
       		.addr = (uintptr_t)crashData
   		};
		ret = nvme_submit_admin_passthru(fd, &vsc_get_dump);
   		if (ret != 0) {
       		printf("Error during admin cmd passthru setup (ret = %d)!\n", ret);
       		free(data);
			free(dataInput);
			free(dataOutput);
			free(crashData);
       		return ret;
   		}
		for(int i =0; i<pageSize;i++)
		{
			fputc(crashData[i],fp);
		}
		free(crashData);
	}
	printf("Finished extracting crash dump\n");
	fclose(fp);
	free(dataInput);
	free(dataOutput);
	free(data);
	return ret;
}

static int vt_set_info(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
   	int fd, ret;
   	int cmd_data_len = 512;
   	int ret_len = 20;
   	const char *serial_num = "";
	const char *model_num = "";
   	const char *serial = "set the serial num";
	const char *model = "set the model num";
   	OPT_ARGS(opts) = {
       	OPT_STRING("serial-num",'s',"SERIAL",&serial_num,serial),
		OPT_STRING("model-num",'n',"MODEL",&model_num,model),
       	OPT_END()
   	};
   	fd = parse_and_open(argc, argv, "", opts);
   	if (fd < 0) {
       	printf("Error parse and open (fd = %d)\n", fd);
       	return -1;
   	}

   	
	if (strcmp(serial_num,"")!=0)
	{
		__u8* data = calloc(cmd_data_len, sizeof(__u8));
		data[0] = 0x26;
   		data[1] = 0x05;
		data[2] = 0x01;

		struct nvme_admin_cmd setup_vsc_set_info = {
       		.opcode = 0xfd,
       		.cdw12 = 0x00fc,
       		.cdw10 = 128,
       		.data_len = cmd_data_len,
       		.addr = (uintptr_t)data
   		};
   		// setup vsc to set serial num
   		ret = nvme_submit_admin_passthru(fd, &setup_vsc_set_info);
   		if (ret != 0) {
       		printf("Error during admin cmd passthru setup (ret = %d)!\n", ret);
       		free(data);
       		return ret;
   		}
		__u8* dataInput = calloc(sizeof(serial_num), sizeof(__u8));
		for (int i = 0; i < strlen(serial_num);i++)
		{
			dataInput[i] = serial_num[i];
		}
		struct nvme_admin_cmd vsc_set_info = {
       		.opcode = 0xfd,
       		.cdw12 = 0x00fd,
       		.cdw10 = 5,
       		.data_len = ret_len,
       		.addr = (uintptr_t)dataInput
   		};
		ret = nvme_submit_admin_passthru(fd, &vsc_set_info);
		if (ret != 0) {
       		printf("Error during admin cmd passthru  (ret = %d)!\n", ret);
       		free(data);
			free(dataInput);
       		return ret;
   		}
		free(data);
		free(dataInput);
	}

   	if (strcmp(model_num,"")!=0)
	{
		ret_len = 40;
		__u8* data = calloc(cmd_data_len, sizeof(__u8));
		data[0] = 0x26;
   		data[1] = 0x05;
		data[2] = 0x00;

		struct nvme_admin_cmd setup_vsc_set_info = {
       		.opcode = 0xfd,
       		.cdw12 = 0x00fc,
       		.cdw10 = 128,
       		.data_len = cmd_data_len,
       		.addr = (uintptr_t)data
   		};
   		// setup vsc to set model num
   		ret = nvme_submit_admin_passthru(fd, &setup_vsc_set_info);
   		if (ret != 0) {
       		printf("Error during admin cmd passthru setup 2(ret = %d)!\n", ret);
       		free(data);
       		return ret;
   		}
		__u8* dataInput = calloc(sizeof(model_num), sizeof(__u8));
		for (int i = 0; i < strlen(model_num);i++)
		{
			dataInput[i] = model_num[i];
		}
		struct nvme_admin_cmd vsc_set_info = {
       		.opcode = 0xfd,
       		.cdw12 = 0x00fd,
       		.cdw10 = 10,
       		.data_len = ret_len,
       		.addr = (uintptr_t)dataInput
   		};
		ret = nvme_submit_admin_passthru(fd, &vsc_set_info);
		if (ret != 0) {
       		printf("Error during admin cmd passthru 2 (ret = %d)!\n", ret);
       		free(data);
			free(dataInput);
       		return ret;
   		}
		free(data);
		free(dataInput);
	}
   	
	return ret;
}

static int vt_get_op_info(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
	int fd, ret;
	int cmd_data_len = 512;
	int ret_len = 4;
    
	OPT_ARGS(opts) = {
		OPT_END()
	};
    
	fd = parse_and_open(argc, argv, "", opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return -1;
	}
    
	__u8* data = calloc(cmd_data_len, sizeof(__u8));
	data[0] = 0x27;
	data[1] = 0x05;

	struct nvme_admin_cmd setup_vsc_get_op = {
		.opcode = 0xfd,
		.cdw12 = 0x00fc,
		.cdw10 = 128,
		.data_len = cmd_data_len,
		.addr = (uintptr_t)data
	};

	// setup vsc
	ret = nvme_submit_admin_passthru(fd, &setup_vsc_get_op);
	if (ret != 0) {
		printf("Error during admin cmd passthru setup (ret = %d)!\n", ret);
		free(data);
		return ret;
	}

	struct nvme_admin_cmd vsc_get_op = {
		.opcode = 0xfe,
		.cdw12 = 0x00fd,
		.cdw10 = 1,
		.data_len = ret_len,
		.addr = (uintptr_t)data
	};

	// exec vsc and get mn string back
	ret = nvme_submit_admin_passthru(fd, &vsc_get_op);
	if (ret != 0) {
		printf("Error during admin cmd passthru (ret = %d)!\n", ret);
		free(data);
		return ret;
	}
	printf("Over Provisioning Percent: %s\n", data);
    
	free(data);

	return ret;
}

static int vt_set_op_info(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
	int fd, ret;
	int cmd_data_len = 512;
	int ret_len = 10;
    const char *op = "";
   	const char *set_op = "set the over provision percent (0x0 to 0xffffffff)";

	OPT_ARGS(opts) = {
		OPT_STRING("over-provision",'o',"OP",&op,set_op),
		OPT_END()
	};
    
	fd = parse_and_open(argc, argv, "", opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return -1;
	}
    
	__u8* data = calloc(cmd_data_len, sizeof(__u8));
	data[0] = 0x27;
	data[1] = 0x05;
	data[2] = 0x01;
	for (int i = 0; i < strlen(op);i++)
	{
		data[i+3] = op[i];
	}
	struct nvme_admin_cmd setup_vsc_set_op = {
		.opcode = 0xfd,
		.cdw12 = 0x00fc,
		.cdw10 = 128,
		.data_len = cmd_data_len,
		.addr = (uintptr_t)data
	};

	// setup vsc
	ret = nvme_submit_admin_passthru(fd, &setup_vsc_set_op);
	if (ret != 0) {
		printf("Error during admin cmd passthru setup (ret = %d)!\n", ret);
		free(data);
		return ret;
	}

	struct nvme_admin_cmd vsc_set_op = {
		.opcode = 0xfe,
		.cdw12 = 0x00fd,
		.cdw10 = 3,
		.data_len = ret_len,
		.addr = (uintptr_t)data
	};

	// exec vsc and set the op
	ret = nvme_submit_admin_passthru(fd, &vsc_set_op);
	if (ret != 0) {
		printf("Error during admin cmd passthru (ret = %d)!\n", ret);
		free(data);
		return ret;
	}
    
	free(data);

	return ret;
}

static int vt_parse_series32_telemetry(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
	OPT_ARGS(opts) = {
		OPT_END()
	};

	int fd = parse_and_open(argc, argv, "", opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return -1;
	}

	int log_len = 1024;
	unsigned char* log_data = malloc(log_len);
	int err = nvme_get_log(fd, NVME_NSID_ALL, 7, 0, 1, log_len, log_data);

	mav_host_tele_log_data_t data_block;
	memcpy(&data_block, log_data + 512, sizeof(data_block));

	print_tel_data_block(&data_block);

	free(log_data);

	return err;
}

static int vt_parse_series61_vs_info(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
	OPT_ARGS(opts) = {
		OPT_END()
	};

	int fd = parse_and_open(argc, argv, "", opts);
	if (fd < 0) {
		printf("Error parse and open (fd = %d)\n", fd);
		return -1;
	}

	int log_len = 4096;
	unsigned char* log_data = malloc(log_len);
	int err = nvme_get_log(fd, NVME_NSID_ALL, 0xc6, 0, 1, log_len, log_data);

	if (err != 0) {
		printf("Invalid log page access!\n");
	} else {
		series61_vs_logpage_c6h data_block;
		memcpy(&data_block, log_data, sizeof(data_block));
		print_series61_log_c6h(&data_block);
	}

	free(log_data);

	return err;
}
