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

int main(void)
{
	uiInitOptions o;
	uiWindow *w;
	uiBox *b;
	uiButton *btn;
	uiLabel *lbl;

	memset(&o, 0, sizeof(uiInitOptions));
	if (uiInit(&o) != NULL)
		abort();

	w = uiNewWindow("Hello", 375, 500, 0);
	uiWindowSetMargined(w, 1);

	b = uiNewVerticalBox();
	uiBoxSetPadded(b, 1);
	uiWindowSetChild(w, uiControl(b));

	e = uiNewMultilineEntry();
	uiMultilineEntrySetReadOnly(e, 1);

	uiBox *b2 = uiNewVerticalBox();
	uiBoxSetPadded(b2, 0);
		lbl = uiNewLabel("MLinstall");
		uiBoxAppend(b2, uiControl(lbl), 0);

		lbl = uiNewLabel("Tool to help install Magic Lantern");
		uiBoxAppend(b2, uiControl(lbl), 0);
	uiBoxAppend(b, uiControl(b2), 0);

	btn = uiNewButton("Say Something");
	uiButtonOnClicked(btn, saySomething, NULL);
	uiBoxAppend(b, uiControl(btn), 0);

	uiBoxAppend(b, uiControl(e), 1);

	uiWindowOnClosing(w, onClosing, NULL);
	uiControlShow(uiControl(w));
	uiMain();
	return 0;
}
