# Pair two actual terminal capture sets. Inputs remain outside the repository.
# MAP is a JSON array of {Slug,Id,Title,Kind}; DONOR_DIR/gallery.json supplies
# {Slug,Title,Path,Evidence,Pages:[{Image,Svg,Width,Height,...}]}.
cmake_minimum_required(VERSION 3.20)
foreach(required DONOR_DIR PORTABLE_DIR OUTPUT_DIR MAP)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "Supply -D${required}=path")
  endif()
endforeach()
file(READ "${DONOR_DIR}/gallery.json" donor)
file(READ "${MAP}" mapping)
string(JSON count LENGTH "${donor}")
string(JSON mapped LENGTH "${mapping}")
if(NOT count EQUAL mapped)
  message(FATAL_ERROR "Every donor capture must have an explicit mapping")
endif()
file(MAKE_DIRECTORY "${OUTPUT_DIR}")
file(RELATIVE_PATH donor_path "${OUTPUT_DIR}" "${DONOR_DIR}")
file(RELATIVE_PATH portable_path "${OUTPUT_DIR}" "${PORTABLE_DIR}")
math(EXPR last "${count}-1")
foreach(i RANGE 0 ${last})
  string(JSON slug GET "${donor}" ${i} Slug)
  string(JSON mapped_slug GET "${mapping}" ${i} Slug)
  string(JSON id GET "${mapping}" ${i} Id)
  if(NOT slug STREQUAL mapped_slug OR NOT id MATCHES "^[a-z0-9-]+$")
    message(FATAL_ERROR "Mapping mismatch at ${i}")
  endif()
  string(JSON pages LENGTH "${donor}" ${i} Pages)
  math(EXPR page_last "${pages}-1")
  foreach(page RANGE 0 ${page_last})
    foreach(format Image Svg)
      string(JSON original_file GET "${donor}" ${i} Pages ${page} ${format})
      if(NOT EXISTS "${DONOR_DIR}/${original_file}")
        message(FATAL_ERROR "Missing original capture page: ${original_file}")
      endif()
    endforeach()
  endforeach()
  foreach(extension png svg ansi)
    if(NOT EXISTS "${PORTABLE_DIR}/${id}.${extension}")
      message(FATAL_ERROR "Missing real output: ${id}.${extension}")
    endif()
  endforeach()
endforeach()
# Embedded metadata is inert JSON. Escape HTML openings before inserting it.
string(REPLACE "<" "\\u003c" donor "${donor}")
string(REPLACE "<" "\\u003c" mapping "${mapping}")
set(html [==[
<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Terminal comparison · PortableShell</title>
<style>
:root{color-scheme:dark;font-family:system-ui,sans-serif;background:#10131b;color:#e8edf7}*{box-sizing:border-box}body{margin:0}header{padding:26px 32px 20px;border-bottom:1px solid #33384c;background:#181c29}h1{font-size:28px;margin:7px 0}p{color:#abb5ca;line-height:1.6;margin:8px 0}.tag{color:#8de1de;font:12px monospace;letter-spacing:2px}.layout{display:grid;grid-template-columns:264px 1fr}aside{height:calc(100vh - 184px);position:sticky;top:0;overflow:auto;border-right:1px solid #33384c;padding:18px}input,select,button{background:#222838;color:#e8edf7;border:1px solid #46516b;border-radius:7px;padding:9px;font:inherit}input,select{width:100%;margin-bottom:9px}button{cursor:pointer}button:hover{background:#36314f}nav button{display:block;text-align:left;width:100%;font-size:13px;margin:5px 0;border-color:transparent}nav button[aria-current=true]{border-color:#bd8cf7;background:#30243e}main{padding:24px;min-width:0}.toolbar{display:flex;gap:10px;align-items:center;flex-wrap:wrap}.count{margin-left:auto;font:13px monospace;color:#eac573}.pair{display:grid;grid-template-columns:1fr 1fr;gap:18px;margin-top:24px}figure{margin:0;background:#191e29;border:1px solid #374158;border-radius:12px;padding:12px;min-width:0}figcaption{padding:8px 5px 14px;font-weight:650;display:flex;justify-content:space-between}.label{font-size:12px;font-weight:400;color:#a5b2c8}img{display:block;width:100%;height:auto;border-radius:8px}a{color:#91dfe7}h2{margin-bottom:8px}small{color:#b8a3d8}.notes{display:grid;grid-template-columns:1fr 1fr;gap:24px;padding:20px 4px}.notes p{font-size:14px}.links{display:flex;gap:18px;margin-top:12px;font-size:13px}.strip{margin-top:16px;display:flex;gap:8px;flex-wrap:wrap}.strip button{font-size:12px}.checks{font-size:13px;color:#b8c8b4}.empty{padding:24px}details{margin-top:14px}summary{cursor:pointer;color:#b9a4e4}@media(max-width:1000px){.layout{grid-template-columns:200px 1fr}.pair,.notes{grid-template-columns:1fr}}@media(max-width:650px){.layout{display:block}aside{height:210px;position:relative}.pair{display:block}figure{margin-bottom:20px}header,main{padding:18px}}
</style>
<header><div class="tag">PORTABLESHELL / PRESENTATION LAB</div><h1>Every captured screen, side by side</h1><p>@count@ original captures paired with actual framework output. Menus, typed forms, editable tables, six-tab controls and scrollable documents.</p><div class="checks">C byte-stream replay · Agave · synthetic PortableShell provider · no remote actions</div></header>
<div class="layout"><aside><input id="search" aria-label="Find a screen" placeholder="Find a screen…"><select id="kind" aria-label="Filter components"><option value="">All components</option><option value="CW_MENU">Menus</option><option value="CW_TABLE">Tables and tabs</option><option value="CW_FORM">Forms</option><option value="CW_DOCUMENT">Documents</option></select><nav id="nav"></nav></aside>
<main><div class="toolbar"><button id="prev">← Previous</button><button id="next">Next →</button><span class="count" id="counter"></span></div><h2 id="title"></h2><small id="path"></small><div class="pair"><figure><figcaption>Original capture <span class="label" id="donor-size"></span></figcaption><a id="original-link"><img id="original" alt="Original terminal capture"></a><div id="pages" class="strip"></div><div class="links"><a id="original-svg">SVG</a><a id="original-ansi">Captured bytes</a></div></figure><figure><figcaption>PortableShell equivalent <span class="label">100 × 34 · fixture</span></figcaption><a id="portable-link"><img id="portable" alt="PortableShell terminal replay"></a><div class="links"><a id="portable-svg">SVG</a><a id="portable-ansi">Captured bytes</a></div></figure></div><div class="notes"><div><h3>Capture provenance</h3><p id="evidence"></p><p>Original output is preserved, including clipping and border defects. Long output retains every captured page. Different terminal sizes are shown at the same card width; open either image for full resolution.</p></div><div><h3 id="component"></h3><p id="implementation"></p><p>These are working UI fixtures, not a PBX port. Data is synthetic. Service lifecycle, provisioning, calls and destructive actions are local callback demonstrations; no network, credentials or system changes are performed.</p></div></div><details><summary>Build and verification</summary><p>All 95 screen definitions open and resize. Automated keys exercise menu destinations, six tabs, value writes, form validation/commit/cancel, shared settings and read-only paste/save protection. The core remains byte-oriented. Examples own all service labels and records.</p><p>Generated by <code>tools/rebuild-service-console.cmake</code> and <code>tools/compare-terminal-galleries.cmake</code>. The forms use an optional overview; numeric settings use number-only presentation. Reports reuse the editor's wrapping/navigation with writes disabled.</p><p>Donor placeholders are represented by typed input forms. Those forms demonstrate UI capability; they do not implement the associated remote protocol. The log comparison uses a scrollable recorded fixture, not a live feed.</p></details></main></div>
<script id="donor-data" type="application/json">@donor@</script><script id="map-data" type="application/json">@mapping@</script>
<script>
const donor=JSON.parse(document.getElementById('donor-data').textContent),mapping=JSON.parse(document.getElementById('map-data').textContent);
const base='@donor_path@/',portable='@portable_path@/';let selected=0;
const $=id=>document.getElementById(id);
const labels={CW_MENU:['Menu / contextual navigation','Numbered menu, keyboard navigation, descriptions and consumer callbacks. All declared menu destinations are exercised by tests.'],CW_TABLE:['Editable table / tabs','Typed provider values, revision-checked writes, inline editing, collapsible border-title groups, semantic colors and a viewport scrollbar. Phone controls share one six-tab panel.'],CW_FORM:['Validated input / form overview','A bordered active field plus a summary of the other fields, password masking, choices, validation, staged commit and cancel. Tab and Shift-Tab revisit fields.'],CW_DOCUMENT:['Read-only document / report','Bordered document view with wrapping, paging, start/end navigation and protected content. It reuses the editor rather than introducing a second text engine.']};
function page(n){const d=donor[selected],p=d.Pages[n];$('original').src=base+p.Image;$('original-link').href=base+p.Image;$('original-svg').href=base+p.Svg;$('original-ansi').href=base+d.Slug+'.ansi';$('donor-size').textContent=p.Width+' × '+p.Height+' · page '+(n+1)+'/'+d.Pages.length;}
function show(i){selected=i;const d=donor[i],m=mapping[i];$('title').textContent=d.Title;$('path').textContent=d.Path;$('counter').textContent=(i+1)+' / '+donor.length;$('evidence').textContent=d.Evidence;$('component').textContent=labels[m.Kind][0];$('implementation').textContent=labels[m.Kind][1];$('portable').src=portable+m.Id+'.png';$('portable-link').href=portable+m.Id+'.png';$('portable-svg').href=portable+m.Id+'.svg';$('portable-ansi').href=portable+m.Id+'.ansi';$('pages').replaceChildren();d.Pages.forEach((p,n)=>{const b=document.createElement('button');b.textContent='Page '+(n+1);b.onclick=()=>page(n);$('pages').append(b)});page(0);history.replaceState(null,'','#'+d.Slug);navigation();}
function navigation(){const q=$('search').value.toLowerCase(),kind=$('kind').value;$('nav').replaceChildren();mapping.forEach((m,i)=>{if((kind&&m.Kind!==kind)||!(m.Title+' '+donor[i].Title+' '+donor[i].Path).toLowerCase().includes(q))return;const b=document.createElement('button');b.textContent=(i+1)+'. '+donor[i].Title;b.setAttribute('aria-current',i===selected);b.onclick=()=>show(i);$('nav').append(b)});}
$('search').oninput=navigation;$('kind').onchange=navigation;$('prev').onclick=()=>show((selected+donor.length-1)%donor.length);$('next').onclick=()=>show((selected+1)%donor.length);document.addEventListener('keydown',e=>{if(['INPUT','SELECT'].includes(document.activeElement.tagName))return;if(e.key==='ArrowLeft')$('prev').click();if(e.key==='ArrowRight')$('next').click();});
show(Math.max(0,donor.findIndex(d=>d.Slug===location.hash.slice(1))));
</script></html>
]==])
string(CONFIGURE "${html}" html @ONLY)
file(WRITE "${OUTPUT_DIR}/index.html" "${html}")
message(STATUS "Verified ${count} pairs: ${OUTPUT_DIR}/index.html")
