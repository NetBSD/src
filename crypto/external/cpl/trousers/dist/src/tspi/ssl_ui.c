
#include <sys/types.h>
#include <string.h>

#include <openssl/ui.h>

#include "trousers/tss.h"
#include "spi_utils.h"

static TSS_RESULT do_ui(BYTE *string, UINT32 *string_len, BYTE *popup, int verify)
{
	char pin_buf[UI_MAX_SECRET_STRING_LENGTH + 1];
	char verify_buf[UI_MAX_SECRET_STRING_LENGTH + 1];
	char *popup_nl;
	UI *ui;
	BYTE *unicode;
	TSS_RESULT ret = TSS_E_FAIL;

	popup_nl = malloc(strlen((char *)popup) + 2);
	if (!popup_nl)
		return TSS_E_OUTOFMEMORY;

	ui = UI_new();
	if (!ui)
		goto no_ui;

	sprintf(popup_nl, "%s\n", (char *)popup);
	if (!UI_add_info_string(ui, popup_nl)) {
		printf("add info fail\n");
		goto out;
	}

	/* UI_add_input_string() doesn't count for the null terminator in its last */
	/* parameter, that's why we statically allocated 1 more byte to pin_buf	   */
	if (!UI_add_input_string(ui, "Enter PIN:", 0, pin_buf, 1, UI_MAX_SECRET_STRING_LENGTH)) {
		printf("add input fail\n");
		goto out;
	}

	if (verify &&
	    !UI_add_verify_string(ui, "Verify PIN:", 0, verify_buf, 1, UI_MAX_SECRET_STRING_LENGTH, pin_buf)) {
		printf("Add verify fail\n");
		goto out;
	}

	if (UI_process(ui))
		goto out;

	ret = TSS_SUCCESS;

	unicode = Trspi_Native_To_UNICODE((BYTE *)pin_buf, string_len);
	memset(string, 0, UI_MAX_SECRET_STRING_LENGTH);
	memcpy(string, unicode, *string_len);
 out:
	UI_free(ui);
 no_ui:
	free(popup_nl);
	return ret;
}

/*
 * DisplayPINWindow()
 *
 * Popup the dialog to collect an existing password.
 *
 * string - buffer that the password will be passed back to caller in
 * popup - UTF-8 string to be displayed in the title bar of the dialog box
 *
 */
TSS_RESULT DisplayPINWindow(BYTE *string, UINT32 *string_len, BYTE *popup)
{
	return do_ui(string, string_len, popup, 0);
}
/*
 * DisplayNewPINWindow()
 *
 * Popup the dialog to collect a new password.
 *
 * string - buffer that the password will be passed back to caller in
 * popup - UTF-8 string to be displayed in the title bar of the dialog box
 *
 */
TSS_RESULT DisplayNewPINWindow(BYTE *string, UINT32 *string_len, BYTE *popup)
{
	return do_ui(string, string_len, popup, 1);
}
