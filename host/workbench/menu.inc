static int menu_command(ps_session *s,void *context){return ps_execute(s,(const char*)context);}
static const ps_menu_item menu_workspaces[]={
    {"Tasks",0,menu_command,(void*)"board tasks",0},
    {"Controls",0,menu_command,(void*)"controls",0},
    {"Variables",0,menu_command,(void*)"board variables",0}};
static const ps_menu workspace_menu={"Workspaces",menu_workspaces,3};
static const ps_menu_item menu_colors[]={
    {"Default",0,menu_command,(void*)"theme default",0},
    {"Cyan",0,menu_command,(void*)"theme cyan",0},
    {"Violet",0,menu_command,(void*)"theme violet",0},
    {"Red",0,menu_command,(void*)"theme red",0},
    {"One Dark",0,menu_command,(void*)"theme onedark",0}};
static const ps_menu color_menu={"Theme",menu_colors,5};
static const ps_menu_item menu_display[]={
    {"RGB color",0,menu_command,(void*)"display color rgb",0},
    {"ANSI 256 color",0,menu_command,(void*)"display color ansi",0},
    {"Monochrome",0,menu_command,(void*)"display color mono",0},
    {"Unicode characters",0,menu_command,(void*)"display glyphs unicode",0},
    {"ASCII characters",0,menu_command,(void*)"display glyphs ascii",0}};
static const ps_menu display_menu={"Terminal output",menu_display,5};
static const ps_menu_item menu_tools[]={
    {"Terminal status",0,menu_command,(void*)"status",0},
    {"Command help",0,menu_command,(void*)"help",0},
    {"Theme",&color_menu,0,0,0},
    {"Terminal output",&display_menu,0,0,0}};
static const ps_menu tools_menu={"Tools",menu_tools,4};
static const ps_menu_item menu_root_items[]={
    {"Workspaces",&workspace_menu,0,0,0},
    {"Tools",&tools_menu,0,0,0}};
static const ps_menu default_menu={"PortableShell",menu_root_items,2};
