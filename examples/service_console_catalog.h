/* Synthetic service-console consumer catalog. No live provider data. */
#include "console_workspace.h"
static const cw_entry entries_0[]={
 {"Quick setup","Create an extension range and configure transports.","","setup","",CW_ACTION,0,65535,0,0},
 {"Extensions","Inspect registration, edit accounts and resolve synchronization.","","extensions","",CW_ACTION,0,65535,0,0},
 {"Trunks","Configure external SIP connections.","","trunks","",CW_ACTION,0,65535,0,0},
 {"Dialplan","Inspect, generate and review routing rules before applying.","","dialplan","",CW_ACTION,0,65535,0,0},
 {"Phones","Manage credentials, provisioning and six control tabs.","","phones","",CW_ACTION,0,65535,0,0},
 {"Console / intercom","Dial an extension or a network endpoint.","","console","",CW_ACTION,0,65535,0,0},
 {"Service management","Inspect and request service operations.","","service","",CW_ACTION,0,65535,0,0},
 {"Diagnostics","Run targeted network and registration checks.","","diagnostics","",CW_ACTION,0,65535,0,0},
 {"System status","Inspect provider health and counters.","","status","",CW_ACTION,0,65535,0,0},
 {"Event log","Browse timestamped events.","","logs","",CW_ACTION,0,65535,0,0},
 {"Live console","Inspect console output.","","live-console","",CW_ACTION,0,65535,0,0},
 {"Command guide","Browse commands and parameter forms.","","usage","",CW_ACTION,0,65535,0,0},
 {"Configuration","Edit typed values inline.","","configuration","",CW_ACTION,0,65535,0,0},
 {"System settings","Change mode, debug and reset preferences.","","settings","",CW_ACTION,0,65535,0,0},
 {"Documentation","Read guides without editing their contents.","","docs-list","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_1[]={
 {"Start extension","100","","","",CW_NUMBER,1,99999,0,0},
 {"End extension","105","","","",CW_NUMBER,1,99999,0,0},
 {"Account password","","","","",CW_SECRET,0,65535,0,0},
};
static const cw_entry entries_2[]={
 {"100 · Reception","Database only; registration not observed.","","extension-info","",CW_ACTION,0,65535,0,0},
 {"101 · Office","Database only; registration not observed.","","extension-info","",CW_ACTION,0,65535,0,0},
 {"102 · Support","Database only; registration not observed.","","extension-info","",CW_ACTION,0,65535,0,0},
 {"103 · Sales","Database only; registration not observed.","","extension-info","",CW_ACTION,0,65535,0,0},
 {"130 · Meeting room","Database only; registration not observed.","","extension-info","",CW_ACTION,0,65535,0,0},
 {"150 · Workshop","Database only; registration not observed.","","extension-info","",CW_ACTION,0,65535,0,0},
 {"199 · Lab","Configuration mismatch.","","extension-info","",CW_ACTION,0,65535,0,0},
 {"Add extension","Create a typed account.","","extension-add","",CW_ACTION,0,65535,0,0},
 {"Edit selected","Edit the fixture account.","","extension-edit","",CW_ACTION,0,65535,0,0},
 {"Delete selected","Explicit confirmation is required.","","extension-delete","",CW_ACTION,0,65535,0,0},
 {"Toggle enabled","Application-owned operation.","","","",CW_ACTION,0,65535,0,0},
 {"Synchronize","Review mismatches before reconciliation.","","extension-sync","",CW_ACTION,0,65535,0,0},
 {"Client setup guide","Read registration instructions.","","sip-help","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_3[]={
 {"Add trunk","Connect to an external SIP network.","","trunk-add","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_4[]={
 {"Current dialplan","Read the active routing document.","","dialplan-current","",CW_ACTION,0,65535,0,0},
 {"Generate from extensions","Preview a candidate without applying it.","","dialplan-generated","",CW_ACTION,0,65535,0,0},
 {"Default extension pattern","Preview the _1XX rule.","","dialplan-default","",CW_ACTION,0,65535,0,0},
 {"Apply generated routing","Application-owned operation; simulation only.","","","",CW_ACTION,0,65535,0,0},
 {"Reload routing","Application-owned reload request.","","","",CW_ACTION,0,65535,0,0},
 {"Pattern reference","Read wildcard and priority guidance.","","dialplan-help","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_5[]={
 {"Desk endpoint · 192.0.2.10","Simulated endpoint; no credentials or live discovery.","","phone-details","",CW_ACTION,0,65535,0,0},
 {"Control workspace","Six tabs with typed settings and application actions.","","phone-tab-0","",CW_ACTION,0,65535,0,0},
 {"Add endpoint","Enter a name, address and credentials.","","phone-add","",CW_ACTION,0,65535,0,0},
 {"Edit credentials","Edit a fixture without touching a real phone.","","phone-edit","",CW_ACTION,0,65535,0,0},
 {"Discover endpoints","Explicitly configure discovery first.","","phone-discovery","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_6[]={
 {"Dial extension","Enter a destination; no call starts before submission.","","console-dial","",CW_ACTION,0,65535,0,0},
 {"Play audio to endpoint","Enter an endpoint and audio resource.","","console-audio","",CW_ACTION,0,65535,0,0},
 {"Call endpoint","Enter an IP address and port.","","console-ip","",CW_ACTION,0,65535,0,0},
 {"Answer incoming call","Provider action.","","","",CW_ACTION,0,65535,0,0},
 {"Hang up","Provider action.","","","",CW_ACTION,0,65535,0,0},
 {"Console status","Inspect the console endpoint.","","console-status","",CW_ACTION,0,65535,0,0},
 {"Configure console","Edit signaling preferences.","","configuration","",CW_ACTION,0,65535,0,0},
 {"Active calls","Inspect call state.","","console-calls","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_7[]={
 {"Start service","Request a start through the application provider.","","","",CW_ACTION,0,65535,0,0},
 {"Stop service","Request a stop through the application provider.","","","",CW_ACTION,0,65535,0,0},
 {"Restart service","Request a restart through the application provider.","","","",CW_ACTION,0,65535,0,0},
 {"Service status","Inspect running state and uptime.","","service-status","",CW_ACTION,0,65535,0,0},
 {"Reload signaling","Reload signaling configuration.","","","",CW_ACTION,0,65535,0,0},
 {"Reload dialplan","Reload routing configuration.","","","",CW_ACTION,0,65535,0,0},
 {"Reload all modules","Reload registered modules.","","","",CW_ACTION,0,65535,0,0},
 {"Configure transports","Edit transport preferences.","","configuration","",CW_ACTION,0,65535,0,0},
 {"Endpoints","Inspect configured endpoints.","","service-endpoints","",CW_ACTION,0,65535,0,0},
 {"Transports","Inspect listeners and protocol bindings.","","service-transports","",CW_ACTION,0,65535,0,0},
 {"Active channels","Inspect active channels.","","service-channels","",CW_ACTION,0,65535,0,0},
 {"Registrations","Inspect registration records.","","service-registrations","",CW_ACTION,0,65535,0,0},
 {"Live console","Read console messages.","","live-console","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_8[]={
 {"Health check","Check each service dependency.","","diag-health","",CW_ACTION,0,65535,0,0},
 {"System information","Inspect operating environment.","","diag-system","",CW_ACTION,0,65535,0,0},
 {"SIP port","Inspect the signaling listener.","","diag-sip-port","",CW_ACTION,0,65535,0,0},
 {"Enable SIP debugging","Enable diagnostic output in the provider.","","","",CW_ACTION,0,65535,0,0},
 {"Disable SIP debugging","Disable diagnostic output in the provider.","","","",CW_ACTION,0,65535,0,0},
 {"Extension registration","Enter the extension to check.","","diag-extension","",CW_ACTION,0,65535,0,0},
 {"Trunk connectivity","Enter a trunk identifier.","","diag-trunk","",CW_ACTION,0,65535,0,0},
 {"Call routing","Enter source and destination extensions.","","diag-routing","",CW_ACTION,0,65535,0,0},
 {"Port connectivity","Enter host and port.","","diag-port","",CW_ACTION,0,65535,0,0},
 {"SIP testing suite","Tools, registration, calls and complete checks.","","sip-suite","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_9[]={
 {"Service","Running (fixture)","Health","","",CW_LABEL,0,65535,0,0},
 {"Database","Connected (fixture)","Health","","",CW_LABEL,0,65535,0,0},
 {"Extensions","7","Inventory","","",CW_LABEL,0,65535,0,0},
 {"Registered","0","Inventory","","",CW_LABEL,0,65535,0,0},
 {"Trunks","0","Inventory","","",CW_LABEL,0,65535,0,0},
 {"Active calls","0","Inventory","","",CW_LABEL,0,65535,0,0},
 {"Database-only accounts","6","Synchronization","","",CW_LABEL,0,65535,0,0},
 {"Mismatched accounts","1","Synchronization","","",CW_LABEL,0,65535,0,0},
 {"Review synchronization","","Actions","extension-sync","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_12[]={
 {"List extensions","Inspect all configured accounts.","","extensions","",CW_ACTION,0,65535,0,0},
 {"Create extension","Enter number, name and password.","","usage-form-01","",CW_ACTION,0,65535,0,0},
 {"Extension status","Inspect registration.","","usage-form-02","",CW_ACTION,0,65535,0,0},
 {"List trunks","Inspect connections.","","trunks","",CW_ACTION,0,65535,0,0},
 {"Test trunk","Enter the connection name.","","usage-form-04","",CW_ACTION,0,65535,0,0},
 {"Trunk status","Inspect the connection state.","","usage-form-05","",CW_ACTION,0,65535,0,0},
 {"Service status","Inspect process observations.","","service-status","",CW_ACTION,0,65535,0,0},
 {"Start service","Application request.","","","",CW_ACTION,0,65535,0,0},
 {"Stop service","Application request.","","","",CW_ACTION,0,65535,0,0},
 {"Restart service","Application request.","","","",CW_ACTION,0,65535,0,0},
 {"Reload configuration","Application request.","","","",CW_ACTION,0,65535,0,0},
 {"Test extension","Enter a registration identity.","","usage-form-11","",CW_ACTION,0,65535,0,0},
 {"Test trunk","Enter the trunk name.","","usage-form-12","",CW_ACTION,0,65535,0,0},
 {"Health check","Inspect dependencies.","","diag-health","",CW_ACTION,0,65535,0,0},
 {"System status","Inspect counters.","","status","",CW_ACTION,0,65535,0,0},
 {"Update","Application request.","","","",CW_ACTION,0,65535,0,0},
 {"Event log","Read recent events.","","logs","",CW_ACTION,0,65535,0,0},
};
static const char *const choices_13_3[]={"Development","Production"};
static const cw_entry entries_13[]={
 {"api.base_url","http://localhost:8000","API","","api.base_url",CW_TEXT,0,65535,0,0},
 {"api.port","8000","API","","",CW_NUMBER,1,65535,0,0},
 {"api.enabled","1","API","","",CW_TOGGLE,0,1,0,0},
 {"application.mode","Development","Application","","app.mode",CW_CHOICE,0,65535,choices_13_3,2},
 {"application.debug","1","Application","","app.debug",CW_TOGGLE,0,1,0,0},
 {"application.timezone","UTC","Application","","",CW_TEXT,0,65535,0,0},
 {"signaling.host","192.0.2.1","Signaling","","",CW_TEXT,0,65535,0,0},
 {"signaling.port","5060","Signaling","","",CW_NUMBER,0,65535,0,0},
 {"signaling.transport","default","Signaling","","",CW_TEXT,0,65535,0,0},
 {"signaling.qualify_interval","60","Signaling","","",CW_NUMBER,0,65535,0,0},
 {"signaling.contacts","default","Signaling","","",CW_TEXT,0,65535,0,0},
 {"database.host","192.0.2.1","Database","","",CW_TEXT,0,65535,0,0},
 {"database.port","5060","Database","","",CW_NUMBER,0,65535,0,0},
 {"database.name","default","Database","","",CW_TEXT,0,65535,0,0},
 {"database.username","default","Database","","",CW_TEXT,0,65535,0,0},
 {"database.timeout","60","Database","","",CW_NUMBER,0,65535,0,0},
 {"authentication.algorithm","default","Authentication","","",CW_TEXT,0,65535,0,0},
 {"authentication.expiration","60","Authentication","","",CW_NUMBER,0,65535,0,0},
 {"authentication.refresh_interval","60","Authentication","","",CW_NUMBER,0,65535,0,0},
 {"logging.level","default","Logging","","",CW_TEXT,0,65535,0,0},
 {"logging.channel","default","Logging","","",CW_TEXT,0,65535,0,0},
 {"logging.retention","60","Logging","","",CW_NUMBER,0,65535,0,0},
 {"interface.color","1","Interface","","",CW_TOGGLE,0,1,0,0},
 {"interface.unicode","1","Interface","","",CW_TOGGLE,0,1,0,0},
 {"interface.title","default","Interface","","",CW_TEXT,0,65535,0,0},
 {"interface.mouse","1","Interface","","",CW_TOGGLE,0,1,0,0},
 {"discovery.network","192.0.2.1","Discovery","","",CW_TEXT,0,65535,0,0},
 {"discovery.timeout","60","Discovery","","",CW_NUMBER,0,65535,0,0},
 {"discovery.interval","60","Discovery","","",CW_NUMBER,0,65535,0,0},
 {"provisioning.url","default","Provisioning","","",CW_TEXT,0,65535,0,0},
 {"provisioning.interval","60","Provisioning","","",CW_NUMBER,0,65535,0,0},
 {"provisioning.enabled","1","Provisioning","","",CW_TOGGLE,0,1,0,0},
 {"audio.codec","default","Audio","","",CW_TEXT,0,65535,0,0},
 {"audio.sample_rate","default","Audio","","",CW_TEXT,0,65535,0,0},
 {"audio.volume","default","Audio","","",CW_TEXT,0,65535,0,0},
 {"routing.context","default","Routing","","",CW_TEXT,0,65535,0,0},
 {"routing.pattern","default","Routing","","",CW_TEXT,0,65535,0,0},
 {"routing.timeout","60","Routing","","",CW_NUMBER,0,65535,0,0},
 {"Add configuration","","Actions","config-add","",CW_ACTION,0,65535,0,0},
 {"Edit key and value","","Actions","config-edit","",CW_ACTION,0,65535,0,0},
 {"Reload service","","Actions","","",CW_ACTION,0,65535,0,0},
};
static const char *const choices_14_0[]={"Development","Production"};
static const cw_entry entries_14[]={
 {"Mode","Development","Application","","app.mode",CW_CHOICE,0,65535,choices_14_0,2},
 {"Debug","1","Application","","app.debug",CW_TOGGLE,0,1,0,0},
 {"Set production mode","","Operations","","",CW_ACTION,0,65535,0,0},
 {"Set development mode","","Operations","","",CW_ACTION,0,65535,0,0},
 {"Upgrade service","","Operations","","",CW_ACTION,0,65535,0,0},
 {"Reset configuration","","Operations","reset-menu","",CW_ACTION,0,65535,0,0},
};
static const char *const choices_15_5[]={"UDP","TCP","TLS"};
static const char *const choices_15_6[]={"No","Yes"};
static const cw_entry entries_15[]={
 {"Extension number","100","","","account.number",CW_NUMBER,1,99999,0,0},
 {"Display name","Reception","","","account.name",CW_TEXT,0,65535,0,0},
 {"Password","","","","",CW_SECRET,0,65535,0,0},
 {"Codecs","PCMU,PCMA,G722","","","",CW_TEXT,0,65535,0,0},
 {"Context","internal","","","",CW_TEXT,0,65535,0,0},
 {"Transport","UDP","","","",CW_CHOICE,0,65535,choices_15_5,3},
 {"Direct media","No","","","",CW_CHOICE,0,65535,choices_15_6,2},
 {"Maximum contacts","1","","","",CW_NUMBER,1,32,0,0},
 {"Qualify interval (seconds)","60","","","",CW_NUMBER,0,3600,0,0},
};
static const char *const choices_17_0[]={"Cancel","Delete local fixture"};
static const cw_entry entries_17[]={
 {"Decision","Cancel","","","",CW_CHOICE,0,65535,choices_17_0,2},
};
static const cw_entry entries_18[]={
 {"Number","100","Identity","","",CW_LABEL,0,65535,0,0},
 {"Display name","Reception","Identity","","account.name",CW_TEXT,0,65535,0,0},
 {"Enabled","1","Account","","",CW_TOGGLE,0,1,0,0},
 {"Registration","Not observed","Account","","",CW_LABEL,0,65535,0,0},
 {"Synchronization","Database only","Account","","",CW_LABEL,0,65535,0,0},
 {"Edit account","","Actions","extension-edit","",CW_ACTION,0,65535,0,0},
 {"Test registration","","Actions","extension-registration","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_19[]={
 {"Selected: database → service","Review and reconcile the selected account.","","","",CW_ACTION,0,65535,0,0},
 {"Selected: service → database","Review and import the selected account.","","","",CW_ACTION,0,65535,0,0},
 {"All: database → service","Request a batch reconciliation.","","","",CW_ACTION,0,65535,0,0},
 {"All: service → database","Request a batch import.","","","",CW_ACTION,0,65535,0,0},
 {"Refresh","Reload observations from the provider.","","extension-sync","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_21[]={
 {"Extension","100","","","",CW_NUMBER,1,99999,0,0},
};
static const cw_entry entries_22[]={
 {"README.md","Service console overview and operator guide.","","doc-01","",CW_ACTION,0,65535,0,0},
 {"SIP setup","Registration and codec configuration.","","sip-help","",CW_ACTION,0,65535,0,0},
 {"Dialplan patterns","Routing syntax and examples.","","dialplan-help","",CW_ACTION,0,65535,0,0},
};
static const char *const choices_24_5[]={"UDP","TCP","TLS"};
static const cw_entry entries_24[]={
 {"Trunk name","Upstream","","","",CW_TEXT,0,65535,0,0},
 {"Server","sip.example.invalid","","","",CW_TEXT,0,65535,0,0},
 {"Username","operator","","","",CW_TEXT,0,65535,0,0},
 {"Password","","","","",CW_SECRET,0,65535,0,0},
 {"Port","5060","","","",CW_NUMBER,1,65535,0,0},
 {"Transport","UDP","","","",CW_CHOICE,0,65535,choices_24_5,3},
};
static const cw_entry entries_29[]={
 {"Trunk name","Upstream","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_30[]={
 {"From extension","100","","","",CW_TEXT,0,65535,0,0},
 {"To extension","101","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_31[]={
 {"Host","192.0.2.1","","","",CW_TEXT,0,65535,0,0},
 {"Port","5060","","","",CW_NUMBER,1,65535,0,0},
};
static const cw_entry entries_32[]={
 {"Available tools","Inspect tool availability.","","sip-tools","",CW_ACTION,0,65535,0,0},
 {"Installation guide","Read instructions without installing.","","sip-install","",CW_ACTION,0,65535,0,0},
 {"Registration test","Enter server, account and secret.","","sip-register","",CW_ACTION,0,65535,0,0},
 {"Call test","Enter source and destination.","","sip-call","",CW_ACTION,0,65535,0,0},
 {"Complete suite","Configure the complete diagnostic run.","","sip-full","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_35[]={
 {"Server","192.0.2.1","","","",CW_TEXT,0,65535,0,0},
 {"Extension","100","","","",CW_TEXT,0,65535,0,0},
 {"Password","","","","",CW_SECRET,0,65535,0,0},
 {"Port","5060","","","",CW_NUMBER,1,65535,0,0},
};
static const cw_entry entries_36[]={
 {"Source extension","100","","","",CW_TEXT,0,65535,0,0},
 {"Destination extension","101","","","",CW_TEXT,0,65535,0,0},
 {"Duration (seconds)","10","","","",CW_NUMBER,1,300,0,0},
};
static const cw_entry entries_47[]={
 {"Destination","101","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_48[]={
 {"Endpoint address","192.0.2.10","","","",CW_TEXT,0,65535,0,0},
 {"Audio resource","/media/announcement.wav","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_49[]={
 {"Endpoint address","192.0.2.10","","","",CW_TEXT,0,65535,0,0},
 {"Port","5060","","","",CW_NUMBER,1,65535,0,0},
};
static const cw_entry entries_53[]={
 {"Key","api.base_url","","","",CW_TEXT,0,65535,0,0},
 {"Value","http://localhost:8000","","","api.base_url",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_54[]={
 {"Key","","","","",CW_TEXT,0,65535,0,0},
 {"Value","","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_56[]={
 {"Review reset impact","Confirm explicitly; cancellation is selected initially.","","reset-confirm","",CW_ACTION,0,65535,0,0},
 {"Configuration summary","Inspect counts before making a decision.","","status","",CW_ACTION,0,65535,0,0},
};
static const char *const choices_57_0[]={"Cancel","Reset local fixture"};
static const cw_entry entries_57[]={
 {"Decision","Cancel","","","",CW_CHOICE,0,65535,choices_57_0,2},
};
static const cw_entry entries_58[]={
 {"Extension number","100","","","account.number",CW_NUMBER,1,99999,0,0},
 {"Display name","Reception","","","account.name",CW_TEXT,0,65535,0,0},
 {"Password","","","","",CW_SECRET,0,65535,0,0},
};
static const cw_entry entries_64[]={
 {"Display name","Desk endpoint","","","",CW_TEXT,0,65535,0,0},
 {"Address","192.0.2.10","","","",CW_TEXT,0,65535,0,0},
 {"Username","operator","","","",CW_TEXT,0,65535,0,0},
 {"Password","","","","",CW_SECRET,0,65535,0,0},
};
static const cw_entry entries_66[]={
 {"Address","192.0.2.10","Identity","","",CW_LABEL,0,65535,0,0},
 {"Transport","UDP","Identity","","",CW_LABEL,0,65535,0,0},
 {"State","Fixture; not connected","Observations","","",CW_LABEL,0,65535,0,0},
 {"Open controls","","Actions","phone-tab-0","",CW_ACTION,0,65535,0,0},
 {"Edit credentials","","Actions","phone-edit","",CW_ACTION,0,65535,0,0},
 {"Provision","","Actions","phone-provision","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_68[]={
 {"Extension","100","","","",CW_TEXT,0,65535,0,0},
 {"Server","192.0.2.1","","","",CW_TEXT,0,65535,0,0},
 {"Password","","","","",CW_SECRET,0,65535,0,0},
};
static const cw_entry entries_69[]={
 {"Endpoint","192.0.2.10","Identity","","",CW_LABEL,0,65535,0,0},
 {"Registration","Not connected","Observations","","",CW_LABEL,0,65535,0,0},
 {"Get status","","Actions","phone-details","",CW_ACTION,0,65535,0,0},
 {"Get state","","Actions","phone-details","",CW_ACTION,0,65535,0,0},
 {"Live monitoring","","Actions","status","",CW_ACTION,0,65535,0,0},
 {"Test CTI / SNMP","","Actions","diag-port","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_70[]={
 {"Reboot endpoint","","Operations","","",CW_ACTION,0,65535,0,0},
 {"Factory reset","","Operations","reset-confirm","",CW_ACTION,0,65535,0,0},
 {"Get configuration","","Configuration","configuration","",CW_ACTION,0,65535,0,0},
 {"Set configuration","","Configuration","phone-set-config","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_71[]={
 {"Provision account","","Provisioning","phone-provision","",CW_ACTION,0,65535,0,0},
 {"Remote provisioning","","Provisioning","provision-guide-1","",CW_ACTION,0,65535,0,0},
 {"Event webhook","","Events","provision-guide-2","",CW_ACTION,0,65535,0,0},
 {"Enable CTI","0","Events","","",CW_TOGGLE,0,1,0,0},
};
static const cw_entry entries_72[]={
 {"Accept call","","Call lifecycle","","",CW_ACTION,0,65535,0,0},
 {"Reject call","","Call lifecycle","","",CW_ACTION,0,65535,0,0},
 {"End call","","Call lifecycle","","",CW_ACTION,0,65535,0,0},
 {"Hold","","Call lifecycle","","",CW_ACTION,0,65535,0,0},
 {"Resume","","Call lifecycle","","",CW_ACTION,0,65535,0,0},
 {"Mute microphone","","Controls","cti-guide-05","",CW_ACTION,0,65535,0,0},
 {"Dial number","","Controls","cti-guide-06","",CW_ACTION,0,65535,0,0},
 {"Send DTMF","","Controls","cti-guide-07","",CW_ACTION,0,65535,0,0},
 {"Blind transfer","","Routing","cti-guide-08","",CW_ACTION,0,65535,0,0},
 {"Attended transfer","","Routing","cti-guide-09","",CW_ACTION,0,65535,0,0},
 {"Conference","","Routing","cti-guide-10","",CW_ACTION,0,65535,0,0},
 {"Do not disturb","0","Preferences","","",CW_TOGGLE,0,1,0,0},
 {"Call forward","","Preferences","cti-guide-12","",CW_ACTION,0,65535,0,0},
 {"Display message","","Preferences","cti-guide-13","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_73[]={
 {"Current order","PCMU → PCMA → G722","Negotiation","","",CW_LABEL,0,65535,0,0},
 {"Edit priority","","Actions","codec-order","",CW_ACTION,0,65535,0,0},
 {"Recommended order","","Actions","codec-reordered","",CW_ACTION,0,65535,0,0},
 {"Save changes","","Actions","","",CW_ACTION,0,65535,0,0},
 {"Reset changes","","Actions","codec-reset","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_74[]={
 {"Console call","","Calls","console-ip","",CW_ACTION,0,65535,0,0},
 {"Audio-file call","","Calls","phone-audio-input","",CW_ACTION,0,65535,0,0},
 {"Test call","","Diagnostics","sip-call","",CW_ACTION,0,65535,0,0},
 {"Console status","","Diagnostics","console-status","",CW_ACTION,0,65535,0,0},
 {"Configure console","","Settings","configuration","",CW_ACTION,0,65535,0,0},
 {"Hang up","","Calls","","",CW_ACTION,0,65535,0,0},
 {"Active calls","","Calls","console-calls","",CW_ACTION,0,65535,0,0},
};
static const char *const choices_75_0[]={"Muted","Unmuted"};
static const cw_entry entries_75[]={
 {"Microphone","Muted","","","",CW_CHOICE,0,65535,choices_75_0,2},
};
static const cw_entry entries_77[]={
 {"Digits","123#","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_78[]={
 {"Destination","102","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_79[]={
 {"Consultation destination","102","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_80[]={
 {"Participant","103","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_81[]={
 {"Forward to","104","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_82[]={
 {"Message","Welcome","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_83[]={
 {"Configuration key","display.brightness","","","",CW_TEXT,0,65535,0,0},
 {"Value","75","","","",CW_TEXT,0,65535,0,0},
};
static const cw_entry entries_84[]={
 {"Controller URL","https://provision.example.invalid","","","",CW_TEXT,0,65535,0,0},
 {"Username","operator","","","",CW_TEXT,0,65535,0,0},
 {"Password","","","","",CW_SECRET,0,65535,0,0},
};
static const cw_entry entries_85[]={
 {"URL","https://events.example.invalid/phone","","","",CW_TEXT,0,65535,0,0},
 {"Shared secret","","","","",CW_SECRET,0,65535,0,0},
};
static const char *const choices_86_0[]={"PCMU","PCMA","G722"};
static const char *const choices_86_1[]={"PCMU","PCMA","G722"};
static const char *const choices_86_2[]={"PCMU","PCMA","G722"};
static const cw_entry entries_86[]={
 {"Priority 1","PCMU","Codec priority","","",CW_CHOICE,0,65535,choices_86_0,3},
 {"Priority 2","PCMA","Codec priority","","",CW_CHOICE,0,65535,choices_86_1,3},
 {"Priority 3","G722","Codec priority","","",CW_CHOICE,0,65535,choices_86_2,3},
 {"Move selected up","","Actions","codec-reordered","",CW_ACTION,0,65535,0,0},
 {"Move selected down","","Actions","codec-reordered","",CW_ACTION,0,65535,0,0},
 {"Save order","","Actions","","",CW_ACTION,0,65535,0,0},
 {"Discard changes","","Actions","codec-reset","",CW_ACTION,0,65535,0,0},
};
static const char *const choices_87_0[]={"PCMU","PCMA","G722"};
static const char *const choices_87_1[]={"PCMU","PCMA","G722"};
static const char *const choices_87_2[]={"PCMU","PCMA","G722"};
static const cw_entry entries_87[]={
 {"Priority 1","G722","Codec priority","","",CW_CHOICE,0,65535,choices_87_0,3},
 {"Priority 2","PCMU","Codec priority","","",CW_CHOICE,0,65535,choices_87_1,3},
 {"Priority 3","PCMA","Codec priority","","",CW_CHOICE,0,65535,choices_87_2,3},
 {"Move selected up","","Actions","codec-reordered","",CW_ACTION,0,65535,0,0},
 {"Move selected down","","Actions","codec-reordered","",CW_ACTION,0,65535,0,0},
 {"Save order","","Actions","","",CW_ACTION,0,65535,0,0},
 {"Discard changes","","Actions","codec-reset","",CW_ACTION,0,65535,0,0},
};
static const cw_entry entries_90[]={
 {"Configure credentials","Enter credentials for a selected endpoint.","","phone-selected","",CW_ACTION,0,65535,0,0},
 {"Start simulated scan","No packets are sent by this example.","","","",CW_ACTION,0,65535,0,0},
 {"Refresh results","Return to the local fixture inventory.","","phones","",CW_ACTION,0,65535,0,0},
};
static const cw_screen service_console_screens[]={
 {"main","Service console","SIMULATION · In-memory provider · No remote operations","","","",CW_MENU,entries_0,15,""},
 {"setup","Quick setup","SIMULATION · In-memory provider · No remote operations","main","","",CW_FORM,entries_1,3,""},
 {"extensions","Extensions","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_2,13,""},
 {"trunks","Trunks · no connections configured","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_3,1,""},
 {"dialplan","Dialplan management","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_4,6,""},
 {"phones","Endpoints · discovery is opt-in","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_5,5,""},
 {"console","Console / intercom","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_6,8,""},
 {"service","Service management","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_7,13,""},
 {"diagnostics","Diagnostics","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_8,10,""},
 {"status","System status · simulated observations","SIMULATION · In-memory provider · No remote operations","main","","",CW_TABLE,entries_9,9,""},
 {"logs","Event log · 80 fixture events","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"10:00:00  INFO     Provider snapshot received\n10:01:00  DEBUG    Configuration revision unchanged\n10:02:00  INFO     Endpoint inventory refreshed\n10:03:00  WARN     Registration not observed\n10:04:00  INFO     Provider snapshot received\n10:05:00  DEBUG    Configuration revision unchanged\n10:06:00  INFO     Endpoint inventory refreshed\n10:07:00  WARN     Registration not observed\n10:08:00  INFO     Provider snapshot received\n10:09:00  DEBUG    Configuration revision unchanged\n10:10:00  INFO     Endpoint inventory refreshed\n10:11:00  WARN     Registration not observed\n10:12:00  INFO     Provider snapshot received\n10:13:00  DEBUG    Configuration revision unchanged\n10:14:00  INFO     Endpoint inventory refreshed\n10:15:00  WARN     Registration not observed\n10:16:00  INFO     Provider snapshot received\n10:17:00  DEBUG    Configuration revision unchanged\n10:18:00  INFO     Endpoint inventory refreshed\n10:19:00  WARN     Registration not observed\n10:20:00  INFO     Provider snapshot received\n10:21:00  DEBUG    Configuration revision unchanged\n10:22:00  INFO     Endpoint inventory refreshed\n10:23:00  WARN     Registration not observed\n10:24:00  INFO     Provider snapshot received\n10:25:00  DEBUG    Configuration revision unchanged\n10:26:00  INFO     Endpoint inventory refreshed\n10:27:00  WARN     Registration not observed\n10:28:00  INFO     Provider snapshot received\n10:29:00  DEBUG    Configuration revision unchanged\n10:30:00  INFO     Endpoint inventory refreshed\n10:31:00  WARN     Registration not observed\n10:32:00  INFO     Provider snapshot received\n10:33:00  DEBUG    Configuration revision unchanged\n10:34:00  INFO     Endpoint inventory refreshed\n10:35:00  WARN     Registration not observed\n10:36:00  INFO     Provider snapshot received\n10:37:00  DEBUG    Configuration revision unchanged\n10:38:00  INFO     Endpoint inventory refreshed\n10:39:00  WARN     Registration not observed\n10:40:00  INFO     Provider snapshot received\n10:41:00  DEBUG    Configuration revision unchanged\n10:42:00  INFO     Endpoint inventory refreshed\n10:43:00  WARN     Registration not observed\n10:44:00  INFO     Provider snapshot received\n10:45:00  DEBUG    Configuration revision unchanged\n10:46:00  INFO     Endpoint inventory refreshed\n10:47:00  WARN     Registration not observed\n10:48:00  INFO     Provider snapshot received\n10:49:00  DEBUG    Configuration revision unchanged\n10:50:00  INFO     Endpoint inventory refreshed\n10:51:00  WARN     Registration not observed\n10:52:00  INFO     Provider snapshot received\n10:53:00  DEBUG    Configuration revision unchanged\n10:54:00  INFO     Endpoint inventory refreshed\n10:55:00  WARN     Registration not observed\n10:56:00  INFO     Provider snapshot received\n10:57:00  DEBUG    Configuration revision unchanged\n10:58:00  INFO     Endpoint inventory refreshed\n10:59:00  WARN     Registration not observed\n11:00:00  INFO     Provider snapshot received\n11:01:00  DEBUG    Configuration revision unchanged\n11:02:00  INFO     Endpoint inventory refreshed\n11:03:00  WARN     Registration not observed\n11:04:00  INFO     Provider snapshot received\n11:05:00  DEBUG    Configuration revision unchanged\n11:06:00  INFO     Endpoint inventory refreshed\n11:07:00  WARN     Registration not observed\n11:08:00  INFO     Provider snapshot received\n11:09:00  DEBUG    Configuration revision unchanged\n11:10:00  INFO     Endpoint inventory refreshed\n11:11:00  WARN     Registration not observed\n11:12:00  INFO     Provider snapshot received\n11:13:00  DEBUG    Configuration revision unchanged\n11:14:00  INFO     Endpoint inventory refreshed\n11:15:00  WARN     Registration not observed\n11:16:00  INFO     Provider snapshot received\n11:17:00  DEBUG    Configuration revision unchanged\n11:18:00  INFO     Endpoint inventory refreshed\n11:19:00  WARN     Registration not observed"},
 {"live-console","Console output · recorded fixture","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"Console provider: simulation\nNo shell commands are executed.\n\n10:00:00  INFO     Provider snapshot received\n10:01:00  DEBUG    Configuration revision unchanged\n10:02:00  INFO     Endpoint inventory refreshed\n10:03:00  WARN     Registration not observed\n10:04:00  INFO     Provider snapshot received\n10:05:00  DEBUG    Configuration revision unchanged\n10:06:00  INFO     Endpoint inventory refreshed\n10:07:00  WARN     Registration not observed\n10:08:00  INFO     Provider snapshot received\n10:09:00  DEBUG    Configuration revision unchanged\n10:10:00  INFO     Endpoint inventory refreshed\n10:11:00  WARN     Registration not observed\n10:12:00  INFO     Provider snapshot received\n10:13:00  DEBUG    Configuration revision unchanged\n10:14:00  INFO     Endpoint inventory refreshed\n10:15:00  WARN     Registration not observed\n10:16:00  INFO     Provider snapshot received\n10:17:00  DEBUG    Configuration revision unchanged\n10:18:00  INFO     Endpoint inventory refreshed\n10:19:00  WARN     Registration not observed\n10:20:00  INFO     Provider snapshot received\n10:21:00  DEBUG    Configuration revision unchanged\n10:22:00  INFO     Endpoint inventory refreshed\n10:23:00  WARN     Registration not observed\n10:24:00  INFO     Provider snapshot received\n10:25:00  DEBUG    Configuration revision unchanged\n10:26:00  INFO     Endpoint inventory refreshed\n10:27:00  WARN     Registration not observed\n10:28:00  INFO     Provider snapshot received\n10:29:00  DEBUG    Configuration revision unchanged\n10:30:00  INFO     Endpoint inventory refreshed\n10:31:00  WARN     Registration not observed\n10:32:00  INFO     Provider snapshot received\n10:33:00  DEBUG    Configuration revision unchanged\n10:34:00  INFO     Endpoint inventory refreshed\n10:35:00  WARN     Registration not observed\n10:36:00  INFO     Provider snapshot received\n10:37:00  DEBUG    Configuration revision unchanged\n10:38:00  INFO     Endpoint inventory refreshed\n10:39:00  WARN     Registration not observed\n10:40:00  INFO     Provider snapshot received\n10:41:00  DEBUG    Configuration revision unchanged\n10:42:00  INFO     Endpoint inventory refreshed\n10:43:00  WARN     Registration not observed\n10:44:00  INFO     Provider snapshot received\n10:45:00  DEBUG    Configuration revision unchanged\n10:46:00  INFO     Endpoint inventory refreshed\n10:47:00  WARN     Registration not observed\n10:48:00  INFO     Provider snapshot received\n10:49:00  DEBUG    Configuration revision unchanged\n10:50:00  INFO     Endpoint inventory refreshed\n10:51:00  WARN     Registration not observed\n10:52:00  INFO     Provider snapshot received\n10:53:00  DEBUG    Configuration revision unchanged\n10:54:00  INFO     Endpoint inventory refreshed\n10:55:00  WARN     Registration not observed\n10:56:00  INFO     Provider snapshot received\n10:57:00  DEBUG    Configuration revision unchanged\n10:58:00  INFO     Endpoint inventory refreshed\n10:59:00  WARN     Registration not observed\n11:00:00  INFO     Provider snapshot received\n11:01:00  DEBUG    Configuration revision unchanged\n11:02:00  INFO     Endpoint inventory refreshed\n11:03:00  WARN     Registration not observed\n11:04:00  INFO     Provider snapshot received\n11:05:00  DEBUG    Configuration revision unchanged\n11:06:00  INFO     Endpoint inventory refreshed\n11:07:00  WARN     Registration not observed\n11:08:00  INFO     Provider snapshot received\n11:09:00  DEBUG    Configuration revision unchanged\n11:10:00  INFO     Endpoint inventory refreshed\n11:11:00  WARN     Registration not observed\n11:12:00  INFO     Provider snapshot received\n11:13:00  DEBUG    Configuration revision unchanged\n11:14:00  INFO     Endpoint inventory refreshed\n11:15:00  WARN     Registration not observed\n11:16:00  INFO     Provider snapshot received\n11:17:00  DEBUG    Configuration revision unchanged\n11:18:00  INFO     Endpoint inventory refreshed\n11:19:00  WARN     Registration not observed"},
 {"usage","Command guide","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_12,17,""},
 {"configuration","Configuration","SIMULATION · In-memory provider · No remote operations","main","","",CW_TABLE,entries_13,41,""},
 {"settings","System settings","SIMULATION · In-memory provider · No remote operations","main","","",CW_TABLE,entries_14,6,""},
 {"extension-add","Create extension","SIMULATION · In-memory provider · No remote operations","extensions","","",CW_FORM,entries_15,9,""},
 {"extension-edit","Edit extension","SIMULATION · In-memory provider · No remote operations","extensions","","",CW_FORM,entries_15,9,""},
 {"extension-delete","Delete extension 100?","SIMULATION · In-memory provider · No remote operations","extensions","","",CW_FORM,entries_17,1,""},
 {"extension-info","Extension 100 · Reception","SIMULATION · In-memory provider · No remote operations","extensions","","",CW_TABLE,entries_18,7,""},
 {"extension-sync","Synchronization manager","SIMULATION · In-memory provider · No remote operations","extensions","","",CW_MENU,entries_19,5,""},
 {"sip-help","SIP client setup","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"REGISTER AN ACCOUNT\n\nServer: 192.0.2.1:5060\nUsername: extension number\nPassword: supplied by your administrator\nTransport: UDP, TCP or TLS as configured\n\nCODECS\nPCMU / PCMA: narrowband compatibility\nG722: wideband audio\n\nNAT\nKeep direct media disabled when traversing a firewall. Configure keepalives and verify registration before testing calls."},
 {"extension-registration","Check extension registration","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_FORM,entries_21,1,""},
 {"docs-list","Documentation","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_22,3,""},
 {"doc-01","Operator guide","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"SERVICE CONSOLE\n\nThis consumer composes PortableShell menus, tables, typed forms and a read-only document viewer.\n\nNAVIGATION\nEnter opens an item. Escape returns. Tab switches endpoint pages.\nTable values support inline editing; forms retain staged values until accepted.\n\nPROVIDER BOUNDARY\nAll records in this example are synthetic. No network, filesystem, service, call or device actions are performed.\nA production consumer implements authentication, validation, persistence and asynchronous observations.\n\nSCREEN FAMILIES\nExtensions, trunks, routing, endpoint controls, diagnostics, logs, configuration and documentation share the same framework components."},
 {"trunk-add","Create trunk","SIMULATION · In-memory provider · No remote operations","trunks","","",CW_FORM,entries_24,6,""},
 {"diag-health","Health check","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"CHECK                         RESULT\nDatabase                      PASS (fixture)\nSignaling listener            PASS (fixture)\nRouting configuration         PASS (fixture)\nEndpoint synchronization      WARNING: 7 differences\n\nNEXT STEP\nOpen Extensions → Synchronize to review individual differences.\n\nNo network checks were performed in this demonstration."},
 {"diag-system","System information","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"ENVIRONMENT\nProvider: in-memory fixture\nTransport: byte callbacks\nStorage: caller owned\nProcess execution: not configured\n\nCAPABILITIES\nUnicode, color, menus, forms, editable tables and document navigation.\nOS details belong to the consuming host adapter."},
 {"diag-sip-port","SIP listener check","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"LISTENER\nAddress: 192.0.2.1\nPort: 5060\nTransport: UDP\nObserved state: available (fixture)\n\nA real provider supplies probe completion and error messages asynchronously."},
 {"diag-extension","Check extension registration","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_FORM,entries_21,1,""},
 {"diag-trunk","Check trunk","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_FORM,entries_29,1,""},
 {"diag-routing","Check call routing","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_FORM,entries_30,2,""},
 {"diag-port","Check port connectivity","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_FORM,entries_31,2,""},
 {"sip-suite","SIP testing suite","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_MENU,entries_32,5,""},
 {"sip-tools","Available tools","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"TOOL             STATE\nRegistration     Provider not connected\nCall generator   Provider not connected\nPacket capture   Provider not connected\n\nThe framework never invokes or installs external tools itself."},
 {"sip-install","Tool installation guide","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"INSTALLATION\n\nChoose a SIP diagnostic tool appropriate to the target host.\nConfigure its executable and permissions in the host adapter.\nAttach registration, call and capture callbacks to the consumer.\n\nEmbedded consumers may implement these operations directly over their network stack.\nNo install command is executed by this guide."},
 {"sip-register","SIP test parameters","SIMULATION · In-memory provider · No remote operations","sip-suite","","",CW_FORM,entries_35,4,""},
 {"sip-call","SIP call test","SIMULATION · In-memory provider · No remote operations","sip-suite","","",CW_FORM,entries_36,3,""},
 {"sip-full","SIP test parameters","SIMULATION · In-memory provider · No remote operations","sip-suite","","",CW_FORM,entries_35,4,""},
 {"service-status","Service status","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"UNIT              SIGNALING SERVICE\nState             Running (fixture)\nUptime            2 hours 14 minutes\nConfiguration     Loaded\nReload status     No pending request\n\nThese are simulated observations, not live process state."},
 {"service-endpoints","Signaling endpoints","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"ENDPOINT      TRANSPORT     STATE\n100           UDP           Unregistered\n101           UDP           Unregistered\n199           TCP           Configuration mismatch\n\nFixture records; registration updates are supplied by the application."},
 {"service-transports","Signaling transports","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"NAME          PROTOCOL     BIND\nLocal UDP     UDP          192.0.2.1:5060\nLocal TCP     TCP          192.0.2.1:5060\nSecure        TLS          192.0.2.1:5061\n\nConfiguration preview only."},
 {"service-channels","Active channels","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"No active channels in the local fixture.\n\nThe provider may publish channel identity, caller, destination, state and duration."},
 {"service-registrations","Registrations","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"ACCOUNT       STATE            LAST OBSERVED\n100           Not registered   Fixture snapshot\n101           Not registered   Fixture snapshot\n\nNo registration traffic has been generated."},
 {"dialplan-current","Current routing document","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"[internal]\nexten => _1XX,1,NoOp(Internal call)\n same => n,Dial(PJSIP/${EXTEN},30)\n same => n,Hangup()\n\nRead-only fixture document. Use a provider to load the real active configuration."},
 {"dialplan-generated","Generated routing · not applied","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"[internal]\nexten => 100,1,Dial(PJSIP/100,30)\n same => n,Hangup()\nexten => 101,1,Dial(PJSIP/101,30)\n same => n,Hangup()\n\nCandidate only. The host must validate and explicitly apply it."},
 {"dialplan-default","Default routing pattern · not applied","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"[internal]\nexten => _1XX,1,Dial(PJSIP/${EXTEN},30)\n same => n,Hangup()\n\n_1XX matches extensions 100 through 199.\n\nThis preview does not change active routing."},
 {"dialplan-help","Routing pattern reference","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"PATTERNS\nX       Any digit 0–9\nZ       Any digit 1–9\nN       Any digit 2–9\n[123]   One of the listed digits\n.       One or more characters\n!       Zero or more characters\n\nEXAMPLES\n_1XX    Internal extensions 100–199\n_9X.    External access prefix followed by a number\n\nAlways validate a candidate before applying it."},
 {"console-dial","Dial extension","SIMULATION · In-memory provider · No remote operations","console","","",CW_FORM,entries_47,1,""},
 {"console-audio","Play audio to endpoint","SIMULATION · In-memory provider · No remote operations","console","","",CW_FORM,entries_48,2,""},
 {"console-ip","Call endpoint by address","SIMULATION · In-memory provider · No remote operations","console","","",CW_FORM,entries_49,2,""},
 {"console-status","Console endpoint status","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"Device            Virtual console\nState             Idle (fixture)\nAudio input       Not attached\nAudio output      Not attached\nActive calls      0\n\nAudio and call control are application provider responsibilities."},
 {"console-calls","Active console calls","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"No active calls.\n\nThis example does not originate, accept or terminate real calls."},
 {"config-inline","Configuration · inline edit","SIMULATION · In-memory provider · No remote operations","main","","",CW_TABLE,entries_13,41,""},
 {"config-edit","Edit configuration","SIMULATION · In-memory provider · No remote operations","configuration","","",CW_FORM,entries_53,2,""},
 {"config-add","Add configuration","SIMULATION · In-memory provider · No remote operations","configuration","","",CW_FORM,entries_54,2,""},
 {"config-section","Configuration · collapsed group","SIMULATION · In-memory provider · No remote operations","main","","",CW_TABLE,entries_13,41,""},
 {"reset-menu","Reset configuration","SIMULATION · In-memory provider · No remote operations","settings","","",CW_MENU,entries_56,2,""},
 {"reset-confirm","Reset all simulated configuration?","SIMULATION · In-memory provider · No remote operations","reset-menu","","",CW_FORM,entries_57,1,""},
 {"usage-form-01","Create extension parameters","SIMULATION · In-memory provider · No remote operations","usage","","",CW_FORM,entries_58,3,""},
 {"usage-form-02","Check extension registration","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_FORM,entries_21,1,""},
 {"usage-form-04","Check trunk","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_FORM,entries_29,1,""},
 {"usage-form-05","Check trunk","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_FORM,entries_29,1,""},
 {"usage-form-11","Check extension registration","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_FORM,entries_21,1,""},
 {"usage-form-12","Check trunk","SIMULATION · In-memory provider · No remote operations","diagnostics","","",CW_FORM,entries_29,1,""},
 {"phone-add","Endpoint credentials","SIMULATION · In-memory provider · No remote operations","phones","","",CW_FORM,entries_64,4,""},
 {"phone-selected","Endpoint credentials","SIMULATION · In-memory provider · No remote operations","phones","","",CW_FORM,entries_64,4,""},
 {"phone-details","Desk endpoint","SIMULATION · In-memory provider · No remote operations","phones","","",CW_TABLE,entries_66,6,""},
 {"phone-edit","Endpoint credentials","SIMULATION · In-memory provider · No remote operations","phones","","",CW_FORM,entries_64,4,""},
 {"phone-provision","Provision account","SIMULATION · In-memory provider · No remote operations","phone-tab-2","","",CW_FORM,entries_68,3,""},
 {"phone-tab-0","Endpoint workspace","SIMULATION · In-memory provider · No remote operations","phones","endpoint","Status",CW_TABLE,entries_69,6,""},
 {"phone-tab-1","Endpoint workspace","SIMULATION · In-memory provider · No remote operations","phones","endpoint","Manage",CW_TABLE,entries_70,4,""},
 {"phone-tab-2","Endpoint workspace","SIMULATION · In-memory provider · No remote operations","phones","endpoint","Provision",CW_TABLE,entries_71,4,""},
 {"phone-tab-3","Endpoint workspace","SIMULATION · In-memory provider · No remote operations","phones","endpoint","CTI",CW_TABLE,entries_72,14,""},
 {"phone-tab-4","Endpoint workspace","SIMULATION · In-memory provider · No remote operations","phones","endpoint","Codecs",CW_TABLE,entries_73,5,""},
 {"phone-tab-5","Endpoint workspace","SIMULATION · In-memory provider · No remote operations","phones","endpoint","Calls",CW_TABLE,entries_74,7,""},
 {"cti-guide-05","Mute microphone","SIMULATION · In-memory provider · No remote operations","phone-tab-3","","",CW_FORM,entries_75,1,""},
 {"cti-guide-06","Dial number","SIMULATION · In-memory provider · No remote operations","phone-tab-3","","",CW_FORM,entries_47,1,""},
 {"cti-guide-07","Send DTMF","SIMULATION · In-memory provider · No remote operations","phone-tab-3","","",CW_FORM,entries_77,1,""},
 {"cti-guide-08","Blind transfer","SIMULATION · In-memory provider · No remote operations","phone-tab-3","","",CW_FORM,entries_78,1,""},
 {"cti-guide-09","Attended transfer","SIMULATION · In-memory provider · No remote operations","phone-tab-3","","",CW_FORM,entries_79,1,""},
 {"cti-guide-10","Conference","SIMULATION · In-memory provider · No remote operations","phone-tab-3","","",CW_FORM,entries_80,1,""},
 {"cti-guide-12","Call forwarding","SIMULATION · In-memory provider · No remote operations","phone-tab-3","","",CW_FORM,entries_81,1,""},
 {"cti-guide-13","Display message","SIMULATION · In-memory provider · No remote operations","phone-tab-3","","",CW_FORM,entries_82,1,""},
 {"phone-set-config","Set endpoint configuration","SIMULATION · In-memory provider · No remote operations","phone-tab-1","","",CW_FORM,entries_83,2,""},
 {"provision-guide-1","Remote provisioning","SIMULATION · In-memory provider · No remote operations","phone-tab-2","","",CW_FORM,entries_84,3,""},
 {"provision-guide-2","Event webhook","SIMULATION · In-memory provider · No remote operations","phone-tab-2","","",CW_FORM,entries_85,2,""},
 {"codec-order","Codec priority","SIMULATION · In-memory provider · No remote operations","phone-tab-4","","",CW_TABLE,entries_86,7,""},
 {"codec-reordered","Codec priority","SIMULATION · In-memory provider · No remote operations","phone-tab-4","","",CW_TABLE,entries_87,7,""},
 {"codec-reset","Codec priority","SIMULATION · In-memory provider · No remote operations","phone-tab-4","","",CW_TABLE,entries_86,7,""},
 {"phone-audio-input","Play audio to endpoint","SIMULATION · In-memory provider · No remote operations","console","","",CW_FORM,entries_48,2,""},
 {"phone-discovery","Endpoint discovery","SIMULATION · In-memory provider · No remote operations","phones","","",CW_MENU,entries_90,3,""},
 {"legacy-service","Service management · alternate menu","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_7,13,""},
 {"alternate-diagnostics","Diagnostics · alternate menu","SIMULATION · In-memory provider · No remote operations","main","","",CW_MENU,entries_8,10,""},
 {"setup-complete","Setup complete · simulated","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"✓ Transport definitions prepared\n✓ Extensions 100–105 prepared\n✓ Internal routing prepared\n✓ Local model updated\n\nNo configuration has been written to a server.\nReview Configuration and Extensions before connecting a real provider."},
 {"setup-error","Setup failed · simulated","SIMULATION · In-memory provider · No remote operations","main","","",CW_DOCUMENT,0,0,"SETUP COULD NOT COMPLETE\n\nReason: provider rejected the transport configuration.\nNo changes were committed.\n\nCheck the listener address, permissions and provider response.\nReturn to Quick setup to correct the values and retry."},
};
