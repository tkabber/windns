#include <tcl.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windns.h>
#include <winsock2.h>
#include <iphlpapi.h>

/* Code taken from win/tkWinTest.c of Tk
 *----------------------------------------------------------------------
 *
 * AppendSystemError --
 *
 *	This routine formats a Windows system error message and places
 *	it into the interpreter result.  Originally from tclWinReg.c.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
AppendSystemError(
	Tcl_Interp *interp, /* Current interpreter. */
	DWORD error)        /* Result code from error. */
{
	int length;
	WCHAR *wMsgPtr;
	char *msg;
	char id[TCL_INTEGER_SPACE], msgBuf[24 + TCL_INTEGER_SPACE];
	Tcl_DString ds;
	Tcl_Obj *resultPtr = Tcl_GetObjResult(interp);

	length = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM
		| FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (WCHAR *) &wMsgPtr,
		0, NULL);
	if (length == 0) {
		char *msgPtr;

		length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *) &msgPtr,
			0, NULL);
		if (length > 0) {
			wMsgPtr = (WCHAR *) LocalAlloc(LPTR, (length + 1) * sizeof(WCHAR));
			MultiByteToWideChar(CP_ACP, 0, msgPtr, length + 1, wMsgPtr,
				length + 1);
			LocalFree(msgPtr);
		}
	}
	if (length == 0) {
		if (error == ERROR_CALL_NOT_IMPLEMENTED) {
			msg = "function not supported under Win32s";
		} else {
			sprintf(msgBuf, "unknown error: %ld", error);
			msg = msgBuf;
		}
	} else {
		Tcl_Encoding encoding;

		encoding = Tcl_GetEncoding(NULL, "unicode");
		msg = Tcl_ExternalToUtfDString(encoding, (char *) wMsgPtr, -1, &ds);
		Tcl_FreeEncoding(encoding);
		LocalFree(wMsgPtr);

		length = Tcl_DStringLength(&ds);

		/*
		 * Trim the trailing CR/LF from the system message.
		 */
		if (msg[length-1] == '\n') {
			msg[--length] = 0;
		}
		if (msg[length-1] == '\r') {
			msg[--length] = 0;
		}
	}

	sprintf(id, "%ld", error);
	Tcl_SetErrorCode(interp, "WINDOWS", id, msg, (char *) NULL);
	Tcl_AppendToObj(resultPtr, msg, length);

	if (length != 0) {
		Tcl_DStringFree(&ds);
	}
}

static int
Nameservers_Cmd (
	ClientData clientData,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *const objv[]
	)
{
	if (objc != 1) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return TCL_ERROR;
	}

	DWORD res;
	PFIXED_INFO dataPtr;
	ULONG buflen;
	Tcl_Obj *listObj;
	IP_ADDR_STRING *nextPtr;

	buflen  = sizeof(FIXED_INFO);
	dataPtr = (PFIXED_INFO) ckalloc(buflen);

	res = GetNetworkParams(dataPtr, &buflen);
	if (res == ERROR_BUFFER_OVERFLOW) {
		dataPtr = (PFIXED_INFO) ckrealloc((char*) dataPtr, buflen);
	}
	res = GetNetworkParams(dataPtr, &buflen);

	if (res != ERROR_SUCCESS) {
		Tcl_ResetResult(interp);
		AppendSystemError(interp, res);
		ckfree((char*) dataPtr);
		return TCL_ERROR;
	}

	listObj = Tcl_NewListObj(0, NULL);
	nextPtr = &(dataPtr->DnsServerList);
	while (nextPtr != NULL) {
		Tcl_ListObjAppendElement(interp, listObj,
				Tcl_NewStringObj(nextPtr->IpAddress.String, -1));
		nextPtr = nextPtr->Next;
	}

	Tcl_SetObjResult(interp, listObj);
	ckfree((char*) dataPtr);
	return TCL_OK;

}


int
Windns_Init(Tcl_Interp *interp)
{
    /*
     * This may work with 8.0, but we are using strictly stubs here,
     * which requires 8.1.
     */
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp, "::windns::nameservers", (Tcl_ObjCmdProc *) Nameservers_Cmd,
	    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}
