// Stub for eventual UI migration to generic uilib
/*
Required to use https://github.com/libui-ng/libui-ng
widget/button tooltips
jpeg images
window icons
button sizes?
*/
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ui.h>

uiMultilineEntry *e;

int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

void saySomething(uiButton *b, void *data)
{
	uiMultilineEntryAppend(e, "Saying something\n");
}

void uiButtonSetTooltip(uiButton *b, const char *tooltip);

static uiControl *makeBasicControlsPage(void)
{
	uiBox *vbox;
	uiBox *hbox;
	uiGroup *group;
	uiButton *button;
	uiLabel *label;
	uiForm *entryForm;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	if (1) {
		button = uiNewButton("Make Camera Bootable");
		uiBoxAppend(vbox, uiControl(button), 0);
		button = uiNewButton("Restore Camera Boot Settings");
		uiBoxAppend(vbox, uiControl(button), 0);
	} else {
		label = uiNewLabel("Camera not connected");
		uiBoxAppend(vbox, uiControl(label), 0);
		button = uiNewButton("Connect to camera");
		uiBoxAppend(vbox, uiControl(button), 0);
		
	}

	return uiControl(vbox);
}

int main(void)
{
	uiInitOptions o;
	uiWindow *w;
	uiBox *b;
	uiButton *btn;
	uiLabel *lbl;
	uiTab *tab;
	uiBox *hbox;

	memset(&o, 0, sizeof(uiInitOptions));
	if (uiInit(&o) != NULL)
		abort();

	w = uiNewWindow("MLinstall", 800, 500, 0);
	uiWindowSetMargined(w, 1);

	hbox = uiNewHorizontalBox();
	uiBoxSetPadded(hbox, 1);
	uiWindowSetChild(w, uiControl(hbox));

	b = uiNewVerticalBox();
	uiBoxSetPadded(b, 1);
	uiBoxAppend(hbox, uiControl(b), 1);

	uiBox *b2 = uiNewVerticalBox();
	uiBoxSetPadded(b2, 0);
		lbl = uiNewLabel("MLinstall");
		uiBoxAppend(b2, uiControl(lbl), 0);
		lbl = uiNewLabel("Tool to help install Magic Lantern");
		uiBoxAppend(b2, uiControl(lbl), 0);
	uiBoxAppend(b, uiControl(b2), 0);

	e = uiNewMultilineEntry();
	uiMultilineEntrySetReadOnly(e, 1);
	uiBoxAppend(hbox, uiControl(e), 1);

//	uiMultilineEntryAppend(e, "Welcome to MLinstall\n");
//	uiMultilineEntryAppend(e, "Blah blahasdaspdokaspdokapsodkapsodkaposdkpasdjosifheowejowiejhoiwjoidjwaoeifhawoiecnowaiefnwoafhowuaiefhowijedowief\n");

	uiMultilineEntryAppend(e, "Connected to Canon EOS M\n");
	uiMultilineEntryAppend(e, "Serial number: blah blah\n");
	uiMultilineEntryAppend(e, "Shutter count: 1234\n");
	uiMultilineEntryAppend(e, "Firmware version: 1.2.3\n");
	uiMultilineEntryAppend(e, "Internal Build Version: 5.5.5\n");
	uiMultilineEntryAppend(e, "Current bootflag state: Enabled\n");

	tab = uiNewTab();
	uiBoxAppend(b, uiControl(tab), 1);

	uiTabAppend(tab, "USB", makeBasicControlsPage());
	uiTabSetMargined(tab, 0, 1);

	uiTabAppend(tab, "Memory Card", makeBasicControlsPage());
	uiTabSetMargined(tab, 1, 1);

	uiTabAppend(tab, "About", makeBasicControlsPage());
	uiTabSetMargined(tab, 2, 1);

	uiControlShow(uiControl(tab));

	uiWindowOnClosing(w, onClosing, NULL);
	uiControlShow(uiControl(w));
	uiMain();
	return 0;
}

