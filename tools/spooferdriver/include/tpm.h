#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ── TPM IOCTL codes (from IDA RE of tpm.sys) ────────────────────────── */
#define IOCTL_TPM_CANCEL            0x22C004   /* CancelTpmContext */
#define IOCTL_TPM_SUBMIT_COMMAND    0x22C00C   /* TpmStackSubmit (primary) */
#define IOCTL_TPM_PPI               0x22C014   /* TpmSubmitPPICommand */
#define IOCTL_TPM_DEVICE_INFO       0x22C01C   /* IoctlGetDeviceInfo */
#define IOCTL_TPM_SUBMIT_ALT        0x22C194   /* TpmStackSubmit (alt path) */

/* ── TPM2 Command Codes ──────────────────────────────────────────────── */
#define TPM_CC_ReadPublic           0x00000173
#define TPM_CC_CreatePrimary        0x00000131
#define TPM_CC_Create               0x00000153
#define TPM_CC_Load                 0x00000157
#define TPM_CC_GetCapability        0x0000017A
#define TPM_CC_NV_Read              0x0000014E
#define TPM_CC_NV_ReadPublic        0x00000169
#define TPM_CC_Certify              0x00000148
#define TPM_CC_CertifyCreation      0x0000014A
#define TPM_CC_GetRandom            0x0000017B
#define TPM_CC_ReadClock            0x0000017C
#define TPM_CC_PCR_Read             0x0000017E
#define TPM_CC_Quote                0x00000158
#define TPM_CC_ContextSave          0x00000162

#define TPM_ST_NO_SESSIONS          0x8001
#define TPM_ST_SESSIONS             0x8002

#define TPM_ALG_RSA                 0x0001
#define TPM_ALG_ECC                 0x0023

#define TPM_RC_SUCCESS              0x00000000

#define TPM_NV_EK_CERT_RSA          0x01C00002
#define TPM_NV_EK_NONCE             0x01C00003
#define TPM_NV_EK_TEMPLATE          0x01C00004

#define TPM_EK_HANDLE               0x81010001

#define MAX_RSA_KEY_BYTES           256



typedef struct _TPM2B_PUBLIC_KEY_RSA {
    UINT16  size;
    UINT8   buffer[MAX_RSA_KEY_BYTES];
} TPM2B_PUBLIC_KEY_RSA, *PTPM2B_PUBLIC_KEY_RSA;

typedef struct _TPM_SPOOF_DATA {
    BOOLEAN             Initialized;
    BOOLEAN             HookInstalled;
    TPM2B_PUBLIC_KEY_RSA GeneratedKey;
} TPM_SPOOF_DATA, *PTPM_SPOOF_DATA;



NTSTATUS InitializeTpmSpoofer(UINT8* seed);
NTSTATUS InstallTpmHook(void);
void UninstallTpmHook(void);
void CleanupTpmSpoofer(void);
PTPM_SPOOF_DATA GetTpmSpoofData(void);
BOOLEAN IsTpmSpooferInitialized(void);

#ifdef __cplusplus
}
#endif
