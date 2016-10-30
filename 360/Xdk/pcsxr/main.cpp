#include <xtl.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include "psxcommon.h"
#include "cdriso.h"
#include "r3000a.h"
#include "gui.h"
#include "aurora.h"


//extern "C" void gpuDmaThreadInit();

extern void InitD3D();

void PausePcsx() {

   Config.CpuRunning = 0;
}

void ResumePcsx() {

    Config.CpuRunning = 1;
    // Restart emulation ...
	psxCpu->Execute();
}

void ResetPcsx() {

	Config.CpuRunning = 1;
    SysReset();
	ResumePcsx();
}

void ShutdownPcsx() {
	Config.CpuRunning = 0;
	SysClose();
}


void RunPcsx(char * game) {
	// __SetHWThreadPriorityHigh();

	int res, ret;
	XMemSet(&Config, 0, sizeof(PcsxConfig));
	
	Config.Cpu = CPU_INTERPRETER;

	strcpy(Config.Bios, "SCPH1001.BIN"); // Use actual BIOS
	//strcpy(Config.Bios, "HLE"); // Use HLE
	strcpy(Config.BiosDir, "game:\\BIOS");
	strcpy(Config.Mcd1,"game:\\BIOS\\Memcard1.mcd");
	strcpy(Config.Mcd2,"game:\\BIOS\\Memcard2.mcd");

	Config.PsxOut = 1;
	Config.HLE = 0;
	Config.Xa = 0;  //XA enabled
	Config.Cdda = 0;
	Config.PsxAuto = 0; //Autodetect
	// Config.SlowBoot = 1;
	

	//gpuDmaThreadInit();
	//gpuThreadEnable(false);


	//cdrThreadInit();	
	cdrIsoInit();
	SetIsoFile(game);

	if (SysInit() == -1) 
	{
		printf("SysInit() Error!\n");
		return;
	}

	GPU_clearDynarec(clearDynarec);

	ret = CDR_open();
	if (ret < 0) { SysMessage (_("Error Opening CDR Plugin")); return; }
	ret = GPU_open(NULL);
	if (ret < 0) { SysMessage (_("Error Opening GPU Plugin (%d)"), ret); return; }
	ret = SPU_open(NULL);
	if (ret < 0) { SysMessage (_("Error Opening SPU Plugin (%d)"), ret); return; }
	ret = PAD1_open(NULL);
	if (ret < 0) { SysMessage (_("Error Opening PAD1 Plugin (%d)"), ret); return; }

	CDR_init();
	GPU_init();
	SPU_init();
	PAD1_init(1);
	PAD2_init(2);
	
	SysReset();

	SysPrintf("CheckCdrom\r\n");
	res = CheckCdrom();
	if(res)
		SysPrintf("CheckCdrom: %08x\r\n",res);
	res=LoadCdrom();
	if(res)
		SysPrintf("LoadCdrom: %08x\r\n",res);

	SysPrintf("Execute\r\n");
	// SysReset();

	psxCpu->Execute();
	
}

#ifdef NO_GUI
int main() {
#else
int __main() {
#endif
	InitD3D();
	DWORD dwLaunchDataSize;
	if (XGetLaunchDataSize(&dwLaunchDataSize) == ERROR_SUCCESS) {
		BYTE* pLaunchData = new BYTE[dwLaunchDataSize];
		XGetLaunchData(pLaunchData, dwLaunchDataSize);
		AURORA_LAUNCHDATA_ROM_V2* aurora = (AURORA_LAUNCHDATA_ROM_V2*)pLaunchData;
		char* extracted_path = new char[dwLaunchDataSize];
		memset(extracted_path, 0, dwLaunchDataSize);
		if (aurora->ApplicationId == AURORA_LAUNCHDATA_APPID && aurora->FunctionId == AURORA_LAUNCHDATA_ROM_FUNCID && (aurora->FunctionVersion == 1 || aurora->FunctionVersion == 2)) {
			if (xbox_io_mount("aurora:", aurora->SystemPath) >= 0)
				sprintf_s(extracted_path, dwLaunchDataSize, "aurora:%s%s", aurora->RelativePath, aurora->Exectutable);
			//TODO: mention error
		}
		if (pLaunchData)
			delete []pLaunchData; // Cleanup memory
		if (extracted_path && extracted_path[0] != 0) {
			RunPcsx(extracted_path);
			return 0;
		}
	}
	RunPcsx("game:\\autoboot.iso");
	return 0;
}