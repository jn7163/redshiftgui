#include "common.h"
#include "iup.h"
#include "options.h"
#include "iupgui.h"
#include "iupgui_main.h"
#include "iupgui_gamma.h"
#include "iupgui_settings.h"

// Settings dialog handles
static Ihandle *dialog_settings=NULL;
static Ihandle *listmethod=NULL;
static Ihandle *label_day=NULL;
static Ihandle *label_night=NULL;
static Ihandle *label_transpeed=NULL;
static Ihandle *val_night=NULL;
static Ihandle *val_day=NULL;
static Ihandle *val_transpeed=NULL;
static Ihandle *chk_min=NULL;
static Ihandle *chk_disable=NULL;
static Ihandle *edt_elev=NULL;

// Buffer to hold elevation map value
static char *txt_val=NULL;

// Settings - temp day changed
static int _val_day_changed(Ihandle *ih){
	int val = IupGetInt(ih,"VALUE");
	int rounded = 100*((int)(val/100.0f));
	IupSetfAttribute(label_day,"TITLE","%d� K",rounded);
	guigamma_set_temp(rounded);
	return IUP_DEFAULT;
}

// Settings - temp night changed
static int _val_night_changed(Ihandle *ih){
	int val = IupGetInt(ih,"VALUE");
	int rounded = 100*((int)(val/100.0f));
	IupSetfAttribute(label_night,"TITLE","%d� K",rounded);
	guigamma_set_temp(rounded);
	return IUP_DEFAULT;
}

// Settings - transition speed changed
static int _val_speed_changed(Ihandle *ih){
	int val = IupGetInt(ih,"VALUE");
	IupSetfAttribute(label_transpeed,"TITLE","%d� K/s",val);
	return IUP_DEFAULT;
}

// Setting - cancel
static int _setting_cancel(Ihandle *ih){
	return IUP_CLOSE;
}

// Setting - save
static int _setting_save(Ihandle *ih){
	int vday = 100*((int)(IupGetInt(val_day,"VALUE")/100.0f));
	int vnight = 100*((int)(IupGetInt(val_night,"VALUE")/100.0f));
	char *method = IupGetAttribute(
			listmethod,IupGetAttribute(listmethod,"VALUE"));
	gamma_method_t oldmethod = opt_get_method();
	gamma_method_t newmethod;
	int min = !strcmp(IupGetAttribute(chk_min,"VALUE"),"ON");
	int disable = !strcmp(IupGetAttribute(chk_disable,"VALUE"),"ON");
	char *elev_map = IupGetAttribute(edt_elev,"VALUE");
	int methodsuccess=0;


	LOG(LOGVERBOSE,_("New day temp: %d, new night temp: %d"),vday,vnight);
	if( !(newmethod=gamma_lookup_method(method)) ){
		LOG(LOGERR,_("Invalid method selected"));
	}else{
		if( newmethod != oldmethod ){
			LOG(LOGINFO,_("Gamma method changed to %s"),method);
			gamma_state_free();
			if( !gamma_init_method(opt_get_screen(),opt_get_crtc(),
					newmethod)){
				LOG(LOGERR,_("Unable to set new gamma method, reverting..."));
				if(!gamma_init_method(opt_get_screen(),opt_get_crtc(),
						oldmethod)){
					LOG(LOGERR,_("Unable to revert to old method."));
				}
			}else
				methodsuccess=1;
		}else
			methodsuccess=1;
	}
	if( !methodsuccess ){
		gui_popup(_("Error"),
				_("There was an error setting the new method.\nSettings NOT saved."),
				"ERROR");
		return IUP_CLOSE;
	}
	opt_set_method(newmethod);
	if( !opt_parse_map(elev_map) ){
		gui_popup(_("Error"),
				_("Unable to parse new temperature map,\nPlease try again."),
				"ERROR");
		return IUP_DEFAULT;
	}
	opt_set_min(min);
	opt_set_disabled(disable);
	opt_set_temperatures(vday,vnight);
	opt_set_transpeed(IupGetInt(val_transpeed,"VALUE"));
	opt_write_config();
	return IUP_CLOSE;
}

// Create methods selection frame
static Ihandle *_settings_create_methods(void){
	// Number of methods available
	int avail_methods=0;
	char list_count[3];
	gamma_method_t method;
	char *method_name;
	Ihandle *vbox_method,*frame_method;

	// Method selection
	listmethod = IupList(NULL);
	IupSetAttribute(listmethod,"DROPDOWN","YES");
	IupSetAttribute(listmethod,"EXPAND","HORIZONTAL");
	for( method=GAMMA_METHOD_AUTO; method<GAMMA_METHOD_MAX; ++method){
		method_name = gamma_get_method_name(method);
		if( (strcmp(method_name,"None")!=0) ){
			snprintf(list_count,3,"%d",++avail_methods);
			IupSetAttribute(listmethod,list_count,method_name);
			if( opt_get_method() == method )
				IupSetfAttribute(listmethod,"VALUE","%d",avail_methods);
		}
	}
	vbox_method = IupVbox(
			listmethod,
			NULL);
	frame_method = IupFrame(vbox_method);
	IupSetAttribute(frame_method,"TITLE","Backend");
	return frame_method;
}

// Create day temp slider frame
static Ihandle *_settings_create_day_temp(void){
	Ihandle *vbox_day, *frame_day;
	// Day temperature
	label_day = IupLabel(NULL);
	IupSetfAttribute(label_day,"TITLE",_("%d� K"),opt_get_temp_day());
	val_day = IupVal(NULL);
	IupSetAttribute(val_day,"EXPAND","HORIZONTAL");
	IupSetfAttribute(val_day,"MIN","%d",MIN_TEMP);
	IupSetfAttribute(val_day,"MAX","%d",MAX_TEMP);
	IupSetfAttribute(val_day,"VALUE","%d",opt_get_temp_day());
	IupSetCallback(val_day,"VALUECHANGED_CB",(Icallback)_val_day_changed);
	vbox_day = IupVbox(
			label_day,
			val_day,
			NULL);
	IupSetAttribute(vbox_day,"MARGIN","5");
	frame_day = IupFrame(vbox_day);
	IupSetAttribute(frame_day,"TITLE",_("Day Temperature"));
	return frame_day;
}

// Create night temp slider frame
static Ihandle *_settings_create_night_temp(void){
	Ihandle *vbox_night,*frame_night;
	// Night temperature
	label_night = IupLabel(NULL);
	IupSetfAttribute(label_night,"TITLE",_("%d� K"),opt_get_temp_night());
	val_night = IupVal(NULL);
	IupSetAttribute(val_night,"EXPAND","HORIZONTAL");
	IupSetfAttribute(val_night,"MIN","%d",MIN_TEMP);
	IupSetfAttribute(val_night,"MAX","%d",MAX_TEMP);
	IupSetfAttribute(val_night,"VALUE","%d",opt_get_temp_night());
	IupSetCallback(val_night,"VALUECHANGED_CB",(Icallback)_val_night_changed);
	vbox_night = IupVbox(
			label_night,
			val_night,
			NULL);
	IupSetAttribute(vbox_night,"MARGIN","5");
	frame_night = IupFrame(vbox_night);
	IupSetAttribute(frame_night,"TITLE","Night Temperature");
	return frame_night;
}

// Create transition speed slider frame
static Ihandle *_settings_create_tran(void){
	Ihandle *vbox_transpeed, *frame_speed;
	// Transition speed
	label_transpeed = IupLabel(NULL);
	IupSetfAttribute(label_transpeed,"TITLE",_("%d� K/s"),opt_get_trans_speed());
	val_transpeed = IupVal(NULL);
	IupSetAttribute(val_transpeed,"EXPAND","HORIZONTAL");
	IupSetfAttribute(val_transpeed,"MIN","%d",MIN_SPEED);
	IupSetfAttribute(val_transpeed,"MAX","%d",MAX_SPEED);
	IupSetfAttribute(val_transpeed,"VALUE","%d",opt_get_trans_speed());
	IupSetCallback(val_transpeed,"VALUECHANGED_CB",(Icallback)_val_speed_changed);
	vbox_transpeed = IupVbox(
			label_transpeed,
			val_transpeed,
			NULL);
	IupSetAttribute(vbox_transpeed,"MARGIN","5");
	frame_speed = IupFrame(vbox_transpeed);
	IupSetAttribute(frame_speed,"TITLE","Transition Speed");
	return frame_speed;
}

// Create startup frame
static Ihandle *_settings_create_startup(void){
	Ihandle *frame_startup;
	// Start minimized and/or disabled
	chk_min = IupToggle(_("Start minimized"),NULL);
	IupSetAttribute(chk_min,"EXPAND","HORIZONTAL");
	chk_disable = IupToggle(_("Start disabled"),NULL);
	IupSetAttribute(chk_disable,"EXPAND","HORIZONTAL");
	if(opt_get_min())
		IupSetAttribute(chk_min,"VALUE","ON");
	if(opt_get_disabled())
		IupSetAttribute(chk_disable,"VALUE","ON");
	frame_startup = IupFrame(IupSetAtt(NULL,
					IupVbox(chk_min,chk_disable,NULL),
					"MARGIN","5",NULL)
				);
	IupSetAttribute(frame_startup,"TITLE",_("Startup"));
	return frame_startup;
}

// Create solar elevations frame
static Ihandle *_settings_create_elev(void){
	Ihandle *lbl_elev,*frame_elev;
	int size;
	pair *map = opt_get_map(&size);
	int i;

	// Assume a size of 20 char per line
#define LINE_SIZE 20
	txt_val = (char*)malloc(size*sizeof(char)*LINE_SIZE+1);

	for( i=0; i<size; ++i ){
		// Use up LINE_SIZE of buffer, with NULL terminator being overwritten on
		// next loop
		snprintf(txt_val+LINE_SIZE*i,LINE_SIZE+1,"%9.2f,%7f%%;\n",
				map[i].elev,map[i].temp);
	}
	edt_elev = IupSetAtt(NULL,IupText(NULL),
			"EXPAND","YES","MULTILINE","YES",
			"VISIBLELINES","4",
			"SCROLLBAR","VERTICAL",NULL);
	IupSetAttribute(edt_elev,"VALUE",txt_val);

	lbl_elev = IupSetAtt(NULL,
			IupLabel(_("Enter comma separated list of\n"
					"elevation to temperature pairs,\n"
					"in between values are linearly \n"
					"interpolated.\n"
					"(0% - night, 100% - day temp.)")),"WORDWRAP","YES",NULL);
	frame_elev=IupFrame(IupSetAtt(NULL,
				IupVbox(edt_elev,lbl_elev,NULL),
				"MARGIN","5",NULL)
				);

	IupSetAttribute(frame_elev,"TITLE",_("Temperature map"));
	return frame_elev;
}

// Creates settings dialog
static void _settings_create(void){
	Ihandle *frame_method,
			*frame_day,
			*frame_night,
			*frame_speed,
			*frame_startup,
			*frame_elev,

			*tabs_all,

			*button_cancel,
			*button_save,
			*hbox_buttons,
			*vbox_all;
	extern Ihandle *himg_redshift;

	frame_method = _settings_create_methods();
	frame_day = _settings_create_day_temp();
	frame_night = _settings_create_night_temp();
	frame_speed = _settings_create_tran();
	frame_startup = _settings_create_startup();
	frame_elev = _settings_create_elev();

	// Tabs containing settings
	tabs_all = IupTabs(
			IupVbox(
				frame_method,
				frame_day,
				frame_night,
				frame_startup,
				NULL),
			IupVbox(
				frame_speed,
				frame_elev,
				NULL),
			NULL);
	IupSetAttributes(tabs_all,"TABTITLE0=Basic,"
			"TABTITLE1=Transition");

	// Buttons
	button_cancel = IupButton(_("Cancel"),NULL);
	IupSetCallback(button_cancel,"ACTION",(Icallback)_setting_cancel);
	IupSetfAttribute(button_cancel,"MINSIZE","%dx%d",60,24);
	button_save = IupButton(_("Save"),NULL);
	IupSetCallback(button_save,"ACTION",(Icallback)_setting_save);
	IupSetfAttribute(button_save,"MINSIZE","%dx%d",60,24);
	hbox_buttons = IupHbox(
			button_cancel,
			button_save,
			NULL);

	// Box containing all
	vbox_all = IupVbox(
			tabs_all,
			IupFill(),
			hbox_buttons,
			NULL);
	IupSetfAttribute(vbox_all,"NMARGIN","%dx%d",5,5);
	IupSetAttribute(vbox_all,"ALIGNMENT","ARIGHT");

	dialog_settings=IupDialog(vbox_all);
	IupSetAttribute(dialog_settings,"TITLE",_("Settings"));
	IupSetAttribute(dialog_settings,"RASTERSIZE","250x");
	IupSetAttributeHandle(dialog_settings,"ICON",himg_redshift);
}

// Shows the settings dialog
int guisettings_show(Ihandle *ih){
	if( !dialog_settings )
		_settings_create();
	IupPopup(dialog_settings,IUP_CENTER,IUP_CENTER);
	IupDestroy(dialog_settings);
	if( txt_val )
		free(txt_val);
	dialog_settings = NULL;
	txt_val = NULL;
	guigamma_check(ih);
	if( !guimain_exit_normal() )
		return IUP_CLOSE;
	return IUP_DEFAULT;
}

