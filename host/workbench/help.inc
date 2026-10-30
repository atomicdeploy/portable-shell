static const char *const help_summaries[]={
    "Inspect and adjust connected controls.","Explore commands and their usage.",
    "Open the workspace dashboard.","Edit session notes with save and discard.",
    "Navigate a numbered menu.","Browse session files.",
    "Create, complete and organize tasks.","Store a session value.",
    "Read session values.","Remove a session value.",
    "Transform text.","Calculate with integers.","Hash text in a background job.",
    "Inspect or cancel background work.",
#if PS_OSC
    "Set the terminal window title.","Report terminal progress or directory.",
#endif
    "Ring the terminal bell.",
    "Print text.","Inspect parsed arguments.","Review or clear command history.",
    "Bind a function key to a command.","Choose an appearance palette.","Configure color depth and character output.",
    "Customize prompt text, colors and lines.","Render a seven-segment display.",
    "Clear the terminal.","Show session status.","Close this session."
};
typedef char help_summary_count_matches[(sizeof(help_summaries)/sizeof(help_summaries[0])==sizeof(builtins)/sizeof(builtins[0]))?1:-1];
static const ps_help_item segment_arguments[]={
    {"TEXT","Digits, a minus sign, decimal points and colons. Quote text containing spaces."}
};
static const ps_help_item segment_options[]={
    {"--large","Use larger segments for a display that is easier to read at a distance."},
    {"--red","Use red for active segments."},
    {"--mono","Disable segment colors."},
    {"--ascii","Draw with ASCII characters for terminals without Unicode support."}
};
static const ps_help_item segment_examples[]={
    {"segments 12:48.6","Display a time with fractional seconds."},
    {"segments -23.5 --large --red","Show a large signed reading."},
    {"segments 0123456789 --ascii","Preview all digits on a basic terminal."}
};
static const ps_help_document segment_help={
    "segments","Render a seven-segment display from text. Combine size, color and character options to suit your terminal.",
    segment_arguments,1,segment_options,4,segment_examples,3
};
