// =====================================================================
//  The web app of the ESP32-S3 Lab: HTML (structure), CSS (layout)
//  and JavaScript (what happens). Everything is stored in the flash
//  memory of the ESP32 and is sent to your phone.
// =====================================================================
#pragma once
#include <Arduino.h>

const char WEBPAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32-S3 Lab</title>
<style>
  :root { --bg:#0b1020; --card:#141b2e; --border:#232d47; --text:#e7ecf5; --muted:#8b97b0;
          --accent:#19e3b1; --red:#ff5c7a; --green:#3ddc84; --blue:#7aa2ff; }
  * { box-sizing: border-box; }
  body { margin:0; font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
         background: var(--bg); color: var(--text); -webkit-tap-highlight-color: transparent; }
  header { padding: 16px 16px 6px; max-width: 720px; margin: 0 auto; }
  h1 { margin: 0; font-size: 22px; letter-spacing: -0.01em; }
  .sub { color: var(--muted); font-size: 13px; margin-top: 4px; display: flex; align-items: center; gap: 6px; }
  #dot { width: 8px; height: 8px; border-radius: 50%; background: var(--red); display: inline-block; }
  #dot.on { background: var(--green); }
  nav { display: flex; gap: 6px; overflow-x: auto; padding: 8px 16px; position: sticky; top: 0;
        background: var(--bg); z-index: 5; scrollbar-width: none; max-width: 720px; margin: 0 auto; }
  nav::-webkit-scrollbar { display: none; }
  nav button { flex: 0 0 auto; background: var(--card); color: var(--muted); border: 1px solid var(--border);
               border-radius: 999px; padding: 8px 14px; font-size: 14px; }
  nav button.active { background: var(--accent); color: #04261c; border-color: var(--accent); font-weight: 600; }
  main { padding: 8px 16px 32px; max-width: 720px; margin: 0 auto; }
  .tab { display: none; }
  .tab.active { display: block; }
  .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 10px; }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 14px; padding: 12px 14px; margin-bottom: 10px; }
  .grid .card { margin: 0; }
  .label { color: var(--muted); font-size: 12px; margin-bottom: 6px; }
  .value { font-size: 22px; font-weight: 650; font-variant-numeric: tabular-nums; }
  .accent { color: var(--accent); }
  .row { display: flex; justify-content: space-between; align-items: center; }
  .row .label { margin: 0; }
  canvas.chart { width: 100%; height: 110px; display: block; }
  button { font: inherit; cursor: pointer; }
  .big { width: 100%; background: var(--accent); color: #04261c; border: none; border-radius: 12px;
         padding: 14px; font-size: 17px; font-weight: 650; margin-bottom: 10px; }
  .big:disabled { opacity: 0.55; }
  .buttons { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; }
  .buttons.three { grid-template-columns: repeat(3, 1fr); }
  .buttons button, .wide { background: var(--card); color: var(--text); border: 1px solid var(--border);
                           border-radius: 12px; padding: 12px 8px; font-size: 15px; }
  .card .buttons button { background: #1b2440; }
  .buttons button.active { border-color: var(--accent); color: var(--accent); font-weight: 600; }
  .wide { width: 100%; margin-top: 4px; }
  .danger { border-color: var(--red); color: var(--red); }
  input[type=range] { width: 100%; accent-color: var(--accent); }
  input[type=color] { width: 100%; height: 52px; border: none; background: none; padding: 0; }
  .explain { color: var(--muted); font-size: 13px; line-height: 1.55; }
  .explain b { color: var(--text); }
  .board { display: grid; grid-template-columns: 1fr 64px 1fr; grid-auto-rows: 24px; column-gap: 8px; row-gap: 3px; margin-top: 10px; }
  .pcb { grid-column: 2; grid-row: 1 / span 22; background: #0e3b2e; border: 1px solid #1d5c48; border-radius: 8px;
         display: flex; flex-direction: column; justify-content: space-between; align-items: center; padding: 8px 0;
         color: #9fd8c4; font-size: 10px; text-align: center; }
  .module { width: 50px; height: 64px; background: #c9ced6; color: #222; border-radius: 4px; display: flex;
            align-items: center; justify-content: center; font-weight: 700; font-size: 9px; }
  .usb { width: 24px; height: 10px; background: #c9ced6; border-radius: 3px; margin: 4px auto 0; }
  .pinslot { display: flex; align-items: center; gap: 6px; font-size: 11px; color: var(--muted); }
  .pinslot.left { justify-content: flex-end; }
  .dead { opacity: 0.45; width: 40px; text-align: center; }
  .pinkey { width: 44px; height: 24px; padding: 0; border-radius: 7px; border: 1px solid var(--border); background: #1b2440;
            color: var(--text); font-size: 13px; font-weight: 700; touch-action: none; user-select: none;
            -webkit-user-select: none; outline: 2px solid transparent; outline-offset: 1px; transition: background .12s, box-shadow .12s; }
  .pinkey.on { background: var(--keycolor); color: #04121a; box-shadow: 0 0 14px var(--keycolor); }
  .notename { min-width: 30px; font-variant-numeric: tabular-nums; }
  .pinslot.left .notename { text-align: right; }
  .slider { display: grid; grid-template-columns: 92px 1fr; align-items: center; gap: 10px; margin: 8px 0; font-size: 14px; }
  .slider.withvalue { grid-template-columns: 92px 1fr 52px; }
  .slider.withvalue span:last-child { text-align: right; font-variant-numeric: tabular-nums; color: var(--muted); }
  select { width: 100%; background: #1b2440; color: var(--text); border: 1px solid var(--border); border-radius: 8px; padding: 9px 8px; font: inherit; }
  .transport { display: flex; align-items: center; gap: 12px; margin: 2px 0 6px; }
  .playbtn { flex: 0 0 auto; background: var(--accent); color: #04261c; border: none; border-radius: 12px; padding: 11px 24px; font-size: 17px; font-weight: 700; }
  .playbtn.stop { background: var(--red); color: #fff; }
  #b-status { color: var(--muted); font-size: 14px; }
  #b-status.on { color: var(--accent); font-weight: 600; }
  .pads { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; }
  .pads button { height: 52px; border-radius: 12px; border: 1px solid var(--border); background: #1b2440; color: var(--muted);
                 font-size: 14px; font-weight: 700; text-decoration: line-through; }
  .pads button.on { background: #12453b; border-color: var(--accent); color: var(--text); text-decoration: none; }
  .patterns { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; margin: 8px 0 12px; }
  .patterns button { background: #1b2440; color: var(--text); border: 2px solid var(--border); border-radius: 10px; padding: 7px 0; font-size: 17px; font-weight: 700; }
  .patterns button small { display: block; font-size: 10px; font-weight: 500; color: var(--muted); }
  .patterns button.editing { border-color: var(--text); }
  .patterns button.playing { background: var(--accent); color: #04261c; }
  .patterns button.playing small { color: #04261c; }
  .patterns button.waiting { animation: blink .45s infinite alternate; }
  @keyframes blink { from { background: #1b2440; } to { background: #2f6f60; } }
  .stepgrid { display: grid; grid-template-columns: 54px repeat(16, 1fr); gap: 2px; margin-bottom: 10px; }
  .rowlabel { font-size: 11px; color: var(--muted); background: none; border: none; padding: 0 4px 0 0; text-align: left;
              overflow: hidden; white-space: nowrap; text-overflow: ellipsis; }
  .cell { height: 26px; padding: 0; border: none; border-radius: 3px; background: #1b2440; color: #04261c; font-size: 10px; font-weight: 800; }
  .cell.count { background: #27325a; }
  .cell.full { background: var(--accent); }
  .cell.soft { background: #1d7f68; }
  .cell.melody.full { background: #b48cff; }
  .cell.melody.soft { background: #6f55a8; }
  .cell.extra { box-shadow: inset 0 0 0 2px #ffd166; }
  .cell.gone { opacity: 0.3; }
  .cell.now { outline: 2px solid #fff; outline-offset: -2px; }
  #d-stutter { touch-action: none; user-select: none; -webkit-user-select: none; }
  .buttons button.on { border-color: var(--accent); color: var(--accent); }
  .gamebox { border-radius: 16px; padding: 30px 16px; text-align: center; background: var(--card);
             border: 1px solid var(--border); margin-bottom: 10px; transition: background .1s; }
  .gamebox.red { background: #4a1020; border-color: var(--red); }
  .gamebox.green { background: #0f4a2c; border-color: var(--green); }
  #gm-text { font-size: 42px; font-weight: 750; font-variant-numeric: tabular-nums; }
  #gm-sub { color: var(--muted); margin-top: 6px; line-height: 1.5; }
  #f-canvas { width: 100%; aspect-ratio: 4 / 3; border-radius: 12px; display: block; background: #000; touch-action: manipulation; }
  .toggle { display: flex; align-items: center; gap: 8px; margin: 12px 0 4px; font-size: 15px; }
  .toggle input { width: 20px; height: 20px; accent-color: var(--accent); }
  .network { display: flex; justify-content: space-between; align-items: center; gap: 12px; background: var(--card);
             border: 1px solid var(--border); border-radius: 12px; padding: 10px 14px; margin-bottom: 8px; }
  .name { font-weight: 600; word-break: break-all; }
  .netinfo { color: var(--muted); font-size: 12px; margin-top: 2px; }
  .strength { display: flex; align-items: flex-end; gap: 3px; height: 20px; flex: 0 0 auto; }
  .strength span { width: 5px; background: var(--border); border-radius: 1px; }
  .strength span.full { background: var(--accent); }
  table { width: 100%; border-collapse: collapse; font-size: 14px; }
  th { text-align: left; color: var(--muted); font-weight: 500; padding: 7px 10px 7px 0; vertical-align: top; }
  td { padding: 7px 0; text-align: right; word-break: break-all; }
  tr + tr th, tr + tr td { border-top: 1px solid var(--border); }
  #i-tasks th, #i-tasks td { text-align: right; padding: 5px 0 5px 8px; font-variant-numeric: tabular-nums; font-size: 13px; }
  #i-tasks th:first-child, #i-tasks td:first-child { text-align: left; padding-left: 0; }
  #i-tasks tr:first-child th { color: var(--muted); font-weight: 500; }
  .saverow { display: grid; grid-template-columns: 1fr auto auto; gap: 8px; margin-top: 12px; }
  .saverow button { background: #1b2440; color: var(--text); border: 1px solid var(--border); border-radius: 10px; padding: 9px 14px; font-size: 15px; }
  .bgroup h3 { margin: 0 0 4px; font-size: 16px; }
  .brow { margin-top: 10px; }
  .brow .row { font-size: 14px; }
  .brow .row span:last-child { font-variant-numeric: tabular-nums; font-weight: 650; }
  .bbar { height: 10px; border-radius: 5px; background: #1b2440; margin-top: 5px; overflow: hidden; }
  .bbar div { height: 100%; width: 0; background: var(--accent); border-radius: 5px; transition: width .6s; }
  .brow.busy .row span:last-child { color: var(--muted); font-weight: 500; }
</style>
</head>
<body>

<header>
  <h1>ESP32-S3 Lab</h1>
  <div class="sub"><span id="dot"></span><span id="connection">connecting...</span><span>&middot;</span><span id="wifi-name">ESP32</span></div>
</header>

<nav>
  <button data-tab="dash" class="active">Dashboard</button>
  <button data-tab="led">LED</button>
  <button data-tab="music">Music</button>
  <button data-tab="game">Reaction game</button>
  <button data-tab="fractal">Fractal</button>
  <button data-tab="bench">Benchmark</button>
  <button data-tab="wifi">WiFi scan</button>
  <button data-tab="ble">Bluetooth</button>
  <button data-tab="info">Info</button>
</nav>

<main>

<section id="t-dash" class="tab active">
  <div class="grid">
    <div class="card"><div class="label">Chip temperature</div><div class="value" id="d-temp">-</div></div>
    <div class="card"><div class="label">Uptime</div><div class="value" id="d-up">-</div></div>
    <div class="card"><div class="label">Free RAM</div><div class="value" id="d-heap">-</div></div>
    <div class="card"><div class="label">Free PSRAM</div><div class="value" id="d-psram">-</div></div>
    <div class="card"><div class="label">Web requests per sec</div><div class="value" id="d-req">-</div></div>
    <div class="card"><div class="label">Devices connected</div><div class="value" id="d-dev">-</div></div>
    <div class="card"><div class="label">Core 0 busy</div><div class="value" id="d-cpu0">-</div></div>
    <div class="card"><div class="label">Core 1 busy</div><div class="value" id="d-cpu1">-</div></div>
  </div>
  <div class="card"><div class="label">How busy are the two cores? (<span style="color:#19e3b1">core 0</span>, <span style="color:#ffd166">core 1</span>)</div><canvas class="chart" id="g-cpu"></canvas></div>
  <div class="card"><div class="label">Chip temperature (last 2 minutes)</div><canvas class="chart" id="g-temp"></canvas></div>
  <div class="card"><div class="label">Web requests per second</div><canvas class="chart" id="g-req"></canvas></div>
  <div class="card row"><div class="label">BOOT button</div><div class="value" id="d-button">-</div></div>
  <div class="card row"><div class="label">Connection to this page</div><div class="value" id="d-live" style="font-size:16px">-</div></div>
  <p class="explain">Everything on this page comes straight from your ESP32. Try pressing the BOOT button. Open the Fractal or the Benchmark and watch how busy the cores get.</p>
</section>

<section id="t-led" class="tab">
  <div class="card">
    <div class="label">Color</div>
    <input type="color" id="l-color" value="#00ff64">
    <div class="label" style="margin-top:12px">Brightness: <span id="l-brightv">60</span></div>
    <input type="range" id="l-bright" min="0" max="255" value="60">
  </div>
  <div class="card">
    <div class="label">Effect</div>
    <div class="buttons" id="l-effects"></div>
  </div>
  <p class="explain">The effects run in a separate task on <b>core 0</b>. That way the web server on <b>core 1</b> keeps responding, without a single delay.</p>
</section>

<section id="t-music" class="tab">
  <div class="card">
    <div class="label">Set</div>
    <select id="m-set"></select>
    <p class="explain" id="m-setinfo" style="margin:8px 0 10px"></p>
    <button id="z-sound" class="big">Turn on sound</button>
    <div class="transport"><button id="b-start" class="playbtn">Start</button><span id="b-status">off</span></div>
    <div class="slider withvalue"><span>Tempo</span><input type="range" id="b-tempo" min="90" max="135" value="118"><span id="b-bpm">118</span></div>
    <div class="slider withvalue"><span>Swing</span><input type="range" id="b-swing" min="0" max="60" value="20"><span id="b-swingv">20%</span></div>
    <div class="slider withvalue"><span>Volume</span><input type="range" id="m-volume" min="0" max="100" value="80"><span id="m-volumev">80</span></div>
    <label class="toggle"><input type="checkbox" id="m-ledbeat" checked> LED on the board flashes along with the beat</label>
    <p class="explain">The <b>BOOT button</b> turns the beat on and off. On an iPhone: turn off silent mode. With headphones or a speaker you hear the bass best.</p>
  </div>

  <div class="card">
    <div class="label">Live: tap to turn a part on or off right away</div>
    <div class="pads" id="l-pads"></div>
    <div class="buttons three" style="margin-top:10px"><button id="r-toggle">Random off</button><button id="r-dice">New pattern</button><button id="r-fill">Fill now</button></div>
    <div class="slider withvalue"><span>Wildness</span><input type="range" id="r-wild" min="0" max="100" value="40"><span id="r-wildv">40%</span></div>
    <div class="slider"><span>Chords</span><select id="c-prog"></select></div>
    <div class="slider"><span>Key</span><select id="c-key"></select></div>
    <label class="toggle"><input type="checkbox" id="r-fills" checked> Automatic fill every 4 bars</label>
    <p class="explain"><b>Random</b>: now and then the beat jumps to another pattern, and every bar it plays small variations. <b>Wildness</b> sets how much. <b>New pattern</b> instantly makes a new beat in the pattern you are editing. Chords and key change on the next bar.</p>
  </div>

  <div class="card">
    <div class="label">Patterns: while playing it switches on the next bar</div>
    <div class="patterns" id="b-patterns"></div>
    <div class="stepgrid" id="b-grid"></div>
    <div class="buttons three"><button id="b-clear">Clear pattern</button><button id="b-copy">Copy</button><button id="b-paste" disabled>Paste</button></div>
    <div class="saverow"><select id="sv-slot"></select><button id="sv-save">Save</button><button id="sv-load">Load</button></div>
    <p class="explain" id="sv-info">Save your beat (all 4 patterns, tempo, swing, chords and key) in the flash memory of the ESP32. It stays saved after a restart, and you can also load it on another device.</p>
    <p class="explain">Tap a cell: <b>full</b>, then <b>soft</b>, then off again. The hihat also has <b>rolls</b> (2, 3, 4). For the bass, soft means: one octave higher. When Random is on, <b>yellow</b> shows what is added this bar and <b>faded</b> shows what drops out. Tap a track name to hear the sound.</p>
  </div>

  <div class="card">
    <div class="label">DJ effects</div>
    <div class="slider withvalue"><span>Filter</span><input type="range" id="d-filter" min="-100" max="100" value="0"><span id="d-filterv">off</span></div>
    <div class="buttons three"><button id="d-build">Build-up</button><button id="d-stutter">Stutter</button><button id="d-center">Filter off</button></div>
    <p class="explain">Filter to the left = dull, to the right = thin. <b>Build-up</b>: 2 bars of tension with a drum roll that keeps getting faster, and then the drop. <b>Stutter</b>: hold it down, and the beat keeps stuttering on the same step.</p>
  </div>

  <div class="card">
    <div class="label">Synth on pins 4 to 14 (plays in the same key)</div>
    <div class="slider"><span>Sound</span><select id="z-preset"></select></div>
    <div class="slider"><span>Scale</span><select id="z-scale"></select></div>
    <div class="slider"><span>Float</span><input type="range" id="z-float" min="0" max="100" value="60"></div>
    <div class="slider"><span>Reverb</span><input type="range" id="z-reverb" min="0" max="100" value="70"></div>
    <div class="slider"><span>Echo</span><input type="range" id="z-echo" min="0" max="100" value="40"></div>
    <div class="slider"><span>Brightness</span><input type="range" id="z-bright" min="0" max="100" value="55"></div>
  </div>

  <div class="card">
    <div class="label">Your board, with the antenna at the top and the USB ports at the bottom. You can also hold the keys here on the screen.</div>
    <div class="board" id="z-board"><p class="explain">Loading board...</p></div>
  </div>
  <div class="buttons">
    <button id="z-silence">Silence all</button>
    <button id="z-calib">Recalibrate</button>
  </div>
  <p class="explain">Calibrating: take your hands off the board and then tap the button.</p>
</section>

<section id="t-game" class="tab">
  <div class="gamebox" id="gm-box"><div id="gm-text">Ready?</div><div id="gm-sub"></div></div>
  <button id="gm-start" class="big">Start</button>
  <div class="card"><div class="label">Record (saved in the flash memory, even after restarting)</div><div class="value" id="gm-record">-</div></div>
  <p class="explain">The ESP32 measures your reaction time down to the microsecond. The BOOT button works with an <b>interrupt</b>: the chip records the moment you press, whatever it is doing.</p>
</section>

<section id="t-fractal" class="tab">
  <canvas id="f-canvas"></canvas>
  <p class="explain" id="f-info">Tap the picture to zoom in.</p>
  <div class="buttons three">
    <button id="f-out">Zoom out</button>
    <button id="f-reset">Reset</button>
    <button id="f-compare">1 vs 2 cores</button>
  </div>
  <label class="toggle"><input type="checkbox" id="f-two" checked> Calculate with both cores</label>
  <p class="explain">For every pixel the ESP32 repeats a small calculation, up to 255 times. That is millions of calculations per picture. Your phone only colors it in afterwards. Tap a spot to zoom in there.</p>
</section>

<section id="t-bench" class="tab">
  <button id="bm-start" class="big">Start the benchmark</button>
  <p class="explain" id="bm-info">The ESP32 runs a series of short tests: computing, memory and its special hardware. Each test takes about half a second. Meanwhile, look at the Dashboard to see how busy the cores get.</p>
  <div id="bm-list"></div>
  <div class="card explain" id="bm-conclusion" style="display:none"></div>
</section>

<section id="t-wifi" class="tab">
  <button id="w-scan" class="big">Search for WiFi networks</button>
  <p class="explain">While searching, the connection to your phone may hiccup for a moment. That is normal: the ESP32 briefly listens on other channels.</p>
  <div id="w-list"></div>
</section>

<section id="t-ble" class="tab">
  <button id="bt-scan" class="big">Search for Bluetooth devices</button>
  <p class="explain">Besides WiFi, the ESP32-S3 also has <b>Bluetooth Low Energy</b> (BLE 5). It listens for a few seconds to devices that announce themselves: phones, earbuds, watches, laptops... Many devices change their address every few minutes, so nobody can track them. That is why we only show the name, the maker and the strength. WiFi and Bluetooth share the same antenna: while searching, the connection to your phone may hiccup for a moment.</p>
  <div id="bt-list"></div>
</section>

<section id="t-info" class="tab">
  <div class="card"><table id="i-table"></table></div>
  <div class="card explain"><b>Who does what?</b><br>Core 0: WiFi, Bluetooth, the LED effects and part of the fractal.<br>Core 1: the web server, the live connection, touch, the reaction game and the rest of the fractal.<br>For the fractal, both cores keep taking the next free row. That way neither one is done early and nobody sits waiting.</div>
  <div class="card">
    <div class="label">Tasks running right now (FreeRTOS). Each task is a small program; the chip switches between them lightning fast.</div>
    <table id="i-tasks"></table>
  </div>
  <button id="i-restart" class="wide danger">Restart ESP32</button>
</section>

</main>

<script>
const $ = id => document.getElementById(id);
const EFFECTS = ['Off', 'Solid', 'Breathing', 'Rainbow', 'Disco', 'Fire', 'Police', 'Temperature'];
const THRESHOLD = 20;

let info = null, tab = 'dash', currentEffect = 2, ledInitialized = false;
let prevTaps = null, lastUptime = -1;
const tempData = [], reqData = [], cpu0Data = [], cpu1Data = [];

// ---------------- Tabs ----------------
document.querySelectorAll('nav button').forEach(btn => btn.onclick = () => {
  document.querySelectorAll('nav button, .tab').forEach(x => x.classList.remove('active'));
  btn.classList.add('active');
  tab = btn.dataset.tab;
  $('t-' + tab).classList.add('active');
  if (tab === 'dash') drawCharts();
  if (tab === 'fractal' && !fractalLoaded) drawFractal();
  if (tab === 'info') loadTasks();
});

// ---------------- Helpers ----------------
// Ask the ESP32 for something. kind: 'text', 'json' or 'bin' (bytes). If it takes longer than ms, we stop.
async function askEsp(url, ms = 3000, kind = 'text', options = {}) {
  const stop = new AbortController();
  const t = setTimeout(() => stop.abort(), ms);
  try {
    const r = await fetch(url, Object.assign({signal: stop.signal, cache: 'no-store'}, options));
    if (!r.ok) throw new Error('HTTP ' + r.status);
    // reading the answer also has to fit inside the time limit
    if (kind === 'json') return await r.json();
    if (kind === 'bin') return {data: await r.arrayBuffer(), header: name => r.headers.get(name)};
    return await r.text();
  } finally { clearTimeout(t); }
}
const bytes = n => n >= 1048576 ? (n / 1048576).toFixed(1) + ' MB' : Math.round(n / 1024) + ' kB';
function duration(s) {
  const h = Math.floor(s / 3600), m = Math.floor(s % 3600 / 60), sec = s % 60;
  return (h ? h + 'h ' : '') + (h || m ? m + 'm ' : '') + sec + 's';
}
function setConnected(ok) {
  $('dot').className = ok ? 'on' : '';
  $('connection').textContent = ok ? 'connected' : 'no connection';
}

// ---------------- Live status ----------------
// Way 1: a live connection (WebSocket). The ESP32 sends a message by itself right away when you touch
// a pin or press BOOT. That is much faster than asking over and over.
// Way 2 (backup): if that does not work, we ask for /status every few hundred ms.
let live = null, liveOpen = false, liveLast = 0, roundTrip = 60;
function liveStart() {
  try { live = new WebSocket('ws://' + location.hostname + ':81/'); } catch (e) { setTimeout(liveStart, 3000); return; }
  const ws = live;
  ws.onopen = () => { liveOpen = true; liveLast = performance.now(); ws.send('p' + performance.now().toFixed(1)); };
  ws.onmessage = e => {
    const d = e.data;
    if (d[0] === 'p') {                                    // our own little ping comes back: measure the round trip
      const rt = performance.now() - parseFloat(d.slice(1));
      if (rt >= 0 && rt < 2000) roundTrip = roundTrip * 0.7 + rt * 0.3;
      return;
    }
    let s;
    try { s = JSON.parse(d); } catch (x) { return; }
    s.received = performance.now();
    s.rtt = roundTrip;
    liveLast = s.received;
    setConnected(true);
    handleStatus(s);
  };
  ws.onclose = () => {
    if (live === ws) { live = null; liveOpen = false; releaseAll(); }
    setTimeout(liveStart, 2000);                           // try again later
  };
  ws.onerror = () => { try { ws.close(); } catch (x) {} };
}
// On the Music tab we send a tiny message every 50 ms. That keeps the WiFi of your phone
// "awake", so the messages from the ESP32 arrive right away instead of only after a while.
setInterval(() => { if (tab === 'music' && liveOpen && live) { try { live.send('k'); } catch (e) {} } }, 50);
setInterval(() => {                                        // every second: a little ping (measures the round trip and keeps the line open)
  if (liveOpen && live) { try { live.send('p' + performance.now().toFixed(1)); } catch (e) {} }
  if (liveOpen && performance.now() - liveLast > 3000 && live) live.close();   // quiet for too long: reconnect
}, 1000);
const liveWorks = () => liveOpen && performance.now() - liveLast < 1000;

async function statusLoop() {
  if (!liveWorks()) {
    try {
      const before = performance.now();
      const s = await askEsp('/status', 3000, 'json');
      if (!liveWorks()) {                                  // live again in the meantime? then this message is too old
        s.received = performance.now();
        s.rtt = s.received - before;                       // how long the trip there and back took
        setConnected(true);
        handleStatus(s);
      }
    } catch (e) { setConnected(false); releaseAll(); }
  }
  setTimeout(statusLoop, liveWorks() ? 250 : tab === 'music' ? 70 : tab === 'game' ? 100 : 400);
}

function handleStatus(s) {
  $('d-temp').textContent = s.temp.toFixed(1) + ' \u00b0C';
  $('d-up').textContent = duration(s.uptime);
  $('d-heap').textContent = bytes(s.heap);
  $('d-psram').textContent = (info && info.psramTotal) ? bytes(s.psram) : 'none';
  $('d-req').textContent = s.requests;
  $('d-dev').textContent = s.devices + (s.rssi && s.rssi.length ? ' \u00b7 ' + s.rssi.join(', ') + ' dBm' : '');
  $('d-button').textContent = s.button ? 'pressed' : 'released';
  $('d-button').classList.toggle('accent', s.button);
  const hasCpu = Array.isArray(s.cpu) && s.cpu.length === 2;
  $('d-cpu0').textContent = hasCpu ? Math.round(s.cpu[0]) + '%' : '?';
  $('d-cpu1').textContent = hasCpu ? Math.round(s.cpu[1]) + '%' : '?';
  $('d-live').textContent = liveWorks() ? 'live (WebSocket) \u00b7 ' + Math.round(roundTrip) + ' ms round trip'
                                        : 'normal (asking over and over)';

  if (s.uptime !== lastUptime) {                // keep 1 measurement per second
    lastUptime = s.uptime;
    tempData.push(s.temp);
    reqData.push(s.requests);
    cpu0Data.push(hasCpu ? s.cpu[0] : 0);
    cpu1Data.push(hasCpu ? s.cpu[1] : 0);
    [tempData, reqData, cpu0Data, cpu1Data].forEach(d => { if (d.length > 120) d.shift(); });
    if (tab === 'dash') drawCharts();
  }

  currentEffect = s.effect;
  markEffect(s.effect);
  if (!ledInitialized) {                        // first time: put the sliders in the right place
    ledInitialized = true;
    $('l-color').value = '#' + s.color;
    $('l-bright').value = s.brightness;
    $('l-brightv').textContent = s.brightness;
  }
  updateMusic(s);
  updateGame(s);
}

// ---------------- Charts ----------------
// series: one or more lines [{data, color, text}]. fixed: [min, max] if the scale must stay fixed (e.g. 0-100%).
function drawChart(cv, series, minSpan, fixed) {
  const dpr = window.devicePixelRatio || 1, w = cv.clientWidth, h = cv.clientHeight;
  if (!w) return;
  cv.width = w * dpr; cv.height = h * dpr;
  const c = cv.getContext('2d');
  c.scale(dpr, dpr);
  c.font = '12px system-ui, sans-serif';
  if (series[0].data.length < 2) { c.fillStyle = '#8b97b0'; c.fillText('Waiting for measurements...', 4, h / 2); return; }
  const all = [].concat(...series.map(r => r.data));
  let min = Math.min(...all), max = Math.max(...all);
  if (fixed) { min = fixed[0]; max = fixed[1]; }
  else {
    if (max - min < minSpan) { const m = (max + min) / 2; min = m - minSpan / 2; max = m + minSpan / 2; }
    const margin = (max - min) * 0.15; min -= margin; max += margin;
  }
  c.strokeStyle = 'rgba(255,255,255,0.08)'; c.lineWidth = 1;
  for (let i = 1; i < 4; i++) { const y = Math.round(h * i / 4) + 0.5; c.beginPath(); c.moveTo(0, y); c.lineTo(w, y); c.stroke(); }
  const plotW = w - 70, yOf = v => h - 4 - (v - min) / (max - min) * (h - 8);
  let prevLy = -99;
  series.forEach(({data, color, text}) => {
    const n = data.length - 1;
    if (n < 1) return;
    c.strokeStyle = color; c.lineWidth = 2; c.lineJoin = 'round'; c.beginPath();
    data.forEach((v, i) => { const x = i * plotW / n, y = yOf(v); i ? c.lineTo(x, y) : c.moveTo(x, y); });
    c.stroke();
    const lx = plotW, ly = yOf(data[n]);
    c.fillStyle = color; c.beginPath(); c.arc(lx, ly, 3.5, 0, 6.283); c.fill();
    c.font = '600 13px system-ui, sans-serif';
    let ty = Math.max(14, Math.min(h - 4, ly + 4));
    if (Math.abs(ty - prevLy) < 15) ty = prevLy > h / 2 ? prevLy - 15 : prevLy + 15;   // keep the labels from overlapping
    c.fillText(text, lx + 8, ty);
    prevLy = ty;
  });
}
function drawCharts() {
  const last = d => d.length ? d[d.length - 1] : 0;
  const t = tempData.length ? last(tempData).toFixed(1) + ' \u00b0C' : '';
  const v = reqData.length ? last(reqData) + ' /s' : '';
  drawChart($('g-temp'), [{data: tempData, color: '#19e3b1', text: t}], 2);
  drawChart($('g-req'), [{data: reqData, color: '#7aa2ff', text: v}], 4);
  drawChart($('g-cpu'), [{data: cpu0Data, color: '#19e3b1', text: Math.round(last(cpu0Data)) + '%'},
                         {data: cpu1Data, color: '#ffd166', text: Math.round(last(cpu1Data)) + '%'}], 10, [0, 100]);
}
window.addEventListener('resize', drawCharts);

// ---------------- LED ----------------
EFFECTS.forEach((name, i) => {
  const btn = document.createElement('button');
  btn.textContent = name;
  btn.onclick = () => { currentEffect = i; markEffect(i); sendLed('effect=' + i, true); };
  $('l-effects').appendChild(btn);
});
function markEffect(i) {
  document.querySelectorAll('#l-effects button').forEach((btn, n) => btn.classList.toggle('active', n === i));
}
let lastSend = 0;
function sendLed(q, always) {
  const now = Date.now();
  if (!always && now - lastSend < 80) return;      // not more often than ~12 times per second
  lastSend = now;
  askEsp('/led?' + q).catch(() => {});
}
function sendColor(always) {
  let q = 'color=' + $('l-color').value.slice(1);
  if (currentEffect !== 1 && currentEffect !== 2) { q += '&effect=1'; currentEffect = 1; markEffect(1); }
  sendLed(q, always);
}
$('l-color').addEventListener('input', () => sendColor(false));
$('l-color').addEventListener('change', () => sendColor(true));
$('l-bright').addEventListener('input', () => {
  $('l-brightv').textContent = $('l-bright').value;
  sendLed('brightness=' + $('l-bright').value, false);
});
$('l-bright').addEventListener('change', () => sendLed('brightness=' + $('l-bright').value, true));

// =====================================================================
//  MUSIC: deep house and afro house, playable live
// =====================================================================

// ---------- The board: the pins from top to bottom ----------
const ROW_LEFT = ['3V3', '3V3', 'RST', '4', '5', '6', '7', '15', '16', '17', '18', '8', '3', '46', '9', '10', '11', '12', '13', '14', '5V', 'G'];
const ROW_RIGHT = ['G', 'TX', 'RX', '1', '2', '42', '41', '40', '39', '38', '37', '36', '35', '0', '45', '48', '47', '21', '20', '19', 'G', 'G'];

// ---------- Synth on the pins: scales and sounds ----------
const SCALES = {
  'Dorian': [0, 2, 3, 5, 7, 9, 10],
  'Minor pentatonic': [0, 3, 5, 7, 10],
  'Major pentatonic': [0, 2, 4, 7, 9],
  'Lydian': [0, 2, 4, 6, 7, 9, 11]
};
const SHARPS = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
const FLATS = ['C', 'Db', 'D', 'Eb', 'E', 'F', 'Gb', 'G', 'Ab', 'A', 'Bb', 'B'];
const noteNames = () => [-4, -2, 1, 3, 5, 6].includes(musicKey) ? FLATS : SHARPS;   // flat keys are written with flats
// layers: [pitch x, waveform, detune in cents, volume]
const SOUNDS = {
  'Floating pad': {layers: [[1, 'sawtooth', -9, 0.45], [1, 'sawtooth', 9, 0.45], [2, 'triangle', 3, 0.18], [0.5, 'sine', 0, 0.3]], attack: 1, release: 1, bright: 1},
  'House organ':  {layers: [[1, 'sine', 0, 0.55], [2, 'sine', 0, 0.4], [3, 'sine', 2, 0.25], [4, 'sine', -2, 0.12]], attack: 0.12, release: 0.35, bright: 1.8},
  'Glass bell':   {layers: [[1, 'sine', 0, 0.6], [2, 'sine', 4, 0.22], [3.01, 'sine', 0, 0.16], [5.4, 'sine', 0, 0.07]], pluck: 2.2, release: 0.5, bright: 2.5}
};

// ---------- Chords (written in A; the key shifts everything) ----------
// We leave out the root note: the bass plays that one. That makes them sound jazzy, like in deep house.
const CHORDS = {
  'Am9':   {root: 9, type: 'm9',   notes: [60, 64, 67, 71]},
  'Dm9':   {root: 2, type: 'm9',   notes: [53, 57, 60, 64]},
  'Em9':   {root: 4, type: 'm9',   notes: [55, 59, 62, 66]},
  'Fmaj9': {root: 5, type: 'maj9', notes: [57, 60, 64, 67]},
  'G6':    {root: 7, type: '6',    notes: [59, 62, 64, 67]},
  'Cmaj7': {root: 0, type: 'maj7', notes: [55, 59, 64, 67]}
};
const KEYS = [['E minor', -5], ['F minor', -4], ['F# minor', -3], ['G minor', -2], ['G# minor', -1], ['A minor', 0],
              ['Bb minor', 1], ['B minor', 2], ['C minor', 3], ['C# minor', 4], ['D minor', 5], ['Eb minor', 6]];

// ---------- Drum kits: 10 tracks, the last two are bass and chords ----------
// groups: the big buttons of the Live panel. variation: how freely random may play with a track.
const KITS = {
  deep: {
    tracks: [['kick', 'Kick'], ['clap', 'Clap'], ['rim', 'Rim'], ['hihat', 'Hihat'], ['open', 'Open hat'],
             ['shaker', 'Shaker'], ['ride', 'Ride'], ['conga', 'Conga'], ['bass', 'Bass'], ['chord', 'Chord']],
    groups: {Kick: [0], Clap: [1], Hats: [3, 4, 6], Percussion: [2, 5, 7], Bass: [8], Chords: [9]},
    variation: ['fixed', 'light', 'free', 'free', 'free', 'free', 'free', 'free', 'melody', 'melody'],
    fill: {0: 'x...', 1: 'xoxx', 4: '....', 7: 'oxxo'},
    pinTracks: [0, 1], buildTrack: 1, rollTracks: [3], emptyHihat: '..x...x...x...x.'
  },
  afro: {
    tracks: [['kick', 'Kick'], ['clap', 'Clap'], ['clave', 'Clave'], ['hihat', 'Hihat'], ['open', 'Open hat'],
             ['shaker', 'Shaker'], ['bongo', 'Bongo'], ['conga', 'Conga'], ['bass', 'Bass'], ['chord', 'Chord']],
    groups: {Kick: [0], Clap: [1], Hats: [3, 4], Percussion: [2, 5, 6, 7], Bass: [8], Chords: [9]},
    variation: ['fixed', 'light', 'fixed', 'free', 'free', 'free', 'free', 'free', 'melody', 'melody'],
    fill: {0: 'x...', 1: 'x.xx', 6: 'xoxx', 7: 'oxox'},
    pinTracks: [0, 1], buildTrack: 1, rollTracks: [3], emptyHihat: '..x...x...x...x.'
  }
};

// ---------- The sets ----------
// Pattern: 16 steps per track. x = hit, o = soft (for the bass: one octave higher), 2/3/4 = roll, . = nothing
const SETS = {
  'Deep house: Night': {
    kit: 'deep', bpm: 118, swing: 20, scale: 'Dorian', sound: 'Floating pad', chordStyle: 'stab',
    info: 'Calm and warm, for late in the evening: jazzy chords, a rolling bass and lots of swing.',
    drumSound: {kickStart: 105, kickEnd: 44, kickLength: 0.45, kickClick: 0.3},
    names: ['Intro', 'Groove', 'Deep', 'Break'], breakPattern: 3, peakPattern: 2,
    progressions: {
      'Night: Am9 Dm9 Fmaj9 Em9': ['Am9', 'Dm9', 'Fmaj9', 'Em9'],
      'Floating: Am9 Am9 G6 Fmaj9': ['Am9', 'Am9', 'G6', 'Fmaj9'],
      'Dorian: Dm9 Em9 Am9 Am9': ['Dm9', 'Em9', 'Am9', 'Am9'],
      'Warm: Fmaj9 Em9 Dm9 Cmaj7': ['Fmaj9', 'Em9', 'Dm9', 'Cmaj7']
    },
    patterns: [
      ['x...x...x...x...', '................', '................', '..o...o...o...o.', '................',
       '.o.o.o.o.o.o.o.o', '................', '................', 'x.....x...o...x.', '................'],
      ['x...x...x...x...', '....x.......x...', '.......x......x.', '..x...x...x...x.', '................',
       '.o.o.o.o.o.o.o.o', '................', '................', 'x.....x.o.....x.', '...x......x.....'],
      ['x...x...x...x...', '....x.......x...', '...x..x....x..x.', 'o.x.o.x.o.x.o.x.', '..............x.',
       '.o.o.o.o.o.o.o.o', 'x.o.x.o.x.o.x.o.', '......o.......o.', 'x..o..x...x..o..', '...x..x...x..x..'],
      ['................', '....x.......x...', '...x......x..x..', '..o...o...o...o.', '................',
       '.o.o.o.o.o.o.o.o', '................', 'x.o...x.o.x...o.', 'x...............', 'x.......x.......']
    ],
    templates: [
      ['x...x...x...x...', 'x...x...x...x..o', 'x...x...x..ox...'],
      ['....x.......x...', '....x.......x..o', '....x...o...x...'],
      ['................', '.......x......x.', '...x..x....x..x.', '..x.....x.x.....'],
      ['..x...x...x...x.', 'o.x.o.x.o.x.o.x.', '..o...o...o...o.', 'oxoxoxoxoxoxoxox', '..x.2.x...x.3.x.'],
      ['................', '..............x.', '......x.......x.'],
      ['.o.o.o.o.o.o.o.o', 'oooooooooooooooo', '................'],
      ['................', 'x.o.x.o.x.o.x.o.', '..x...x...x...x.'],
      ['................', '......o.......o.', 'x.o...x.o.x...o.', '..o.x..o..x.o.x.'],
      ['x.....x...o...x.', 'x..o..x...x..o..', '..x...x...x...x.', 'x.......x..o..x.', 'x..x..x...o.x...'],
      ['...x......x.....', '...x..x...x..x..', 'x.....x...x.....', '..x..x....x..x..', 'x...............']
    ]
  },
  'Afro house: Percussion': {
    kit: 'afro', bpm: 122, swing: 14, scale: 'Minor pentatonic', sound: 'Glass bell', chordStyle: 'kalimba',
    info: 'Lots of percussion: congas, bongos, shakers and a clave rhythm, with plucked chords.',
    drumSound: {kickStart: 115, kickEnd: 45, kickLength: 0.4, kickClick: 0.6},
    names: ['Intro', 'Groove', 'Break', 'Peak'], breakPattern: 2, peakPattern: 3,
    progressions: {
      'Sun: Am9 Am9 G6 Fmaj9': ['Am9', 'Am9', 'G6', 'Fmaj9'],
      'Fire: Dm9 Am9 Dm9 Em9': ['Dm9', 'Am9', 'Dm9', 'Em9'],
      'Journey: Am9 Em9 Fmaj9 G6': ['Am9', 'Em9', 'Fmaj9', 'G6'],
      'Rest: Am9 Am9 Am9 Dm9': ['Am9', 'Am9', 'Am9', 'Dm9']
    },
    patterns: [
      ['x...x...x...x...', '................', 'x..x..x...x.x...', '................', '..x...x...x...x.',
       'xooxooxoxooxooxo', '................', '................', '................', '................'],
      ['x...x...x...x...', '....x.......x...', 'x..x..x...x.x...', '................', '..x...x...x...x.',
       'xooxooxoxooxooxo', '..o...o..o....o.', '..o.x..o..x.o.x.', 'x..x..x...x..x..', '..x..x....x..x..'],
      ['................', '................', 'x..x..x...x.x...', '................', '..x...x...x...x.',
       'xooxooxoxooxooxo', 'x.o.x.oo.x.ox.o.', 'x.o.x.oox.o.x.ox', '................', 'x.......x.......'],
      ['x...x...x...x...', '....x.......x...', 'x..x..x...x.x...', 'o.o.o.o.o.o.o.o.', '..x...x...x...x.',
       'xooxooxoxooxooxo', '..o...o..o....o.', '..o.x..o..x.o.xo', 'x..x..x...x..x..', '..x..x....x..x..']
    ],
    templates: [
      ['x...x...x...x...', 'x...x...x...x..o'],
      ['....x.......x...', '................', '....x.......x..o'],
      ['x..x..x...x.x...', 'x..x...x..x.x...', '..x.x...x..x....'],
      ['................', 'o.o.o.o.o.o.o.o.', '..o...o...o...o.', 'oooooooooooooooo'],
      ['..x...x...x...x.', '......x.......x.', '................'],
      ['xooxooxoxooxooxo', 'oxoooxoooxoooxoo', '.o.o.o.o.o.o.o.o'],
      ['................', '..o...o..o....o.', 'x.o.x.oo.x.ox.o.', '...o.o.....o.o..'],
      ['..o.x..o..x.o.x.', 'x.o.x.oox.o.x.ox', '..o...o.x.o...o.', 'o..o..x.o..o..x.'],
      ['x..x..x...x..x..', 'x...x..x..x...x.', 'x..o..x...x..o..', 'x.....x...x.....'],
      ['..x..x....x..x..', 'x.......x.......', '...x...x...x...x', 'x..x..x...x.....']
    ]
  }
};

// ---------- State ----------
let ac = null, volumeNode, master, synthBus, mix, reverb, reverbLevel, echoNode, echoLevel, breathDepth;
let drumBus, drumReverb, fxBus, noise, djLP, djHP, buildHP, riserSource = null, trackGain = [];
let riserGain = null, riserFilter = null, riserStart = 0;
let setName = Object.keys(SETS)[0], set = SETS[setName], kit = KITS[set.kit];
let scale = 'Dorian', sound = 'Floating pad', synthMute = false, musicKey = 0, progression = '';
const settings = {float: 60, reverb: 70, echo: 40, bright: 55, synth: 75, master: 80};
const voices = {}, pinKeys = [], noteLabels = [];
let espOn = [], screenOn = [], tapUntil = [], prevButtonCount = null;

let patterns = [], editPattern = 0, playPattern = 0, nextPattern = -1, clipboard = null;
let barPattern = null, barChord = null, barCount = 0, fillThisBar = false, fillNext = false, fillsOn = true;
let randomOn = false, wildness = 40, groupOff = {}, trackOff = [];
let bpm = 118, swing = 20, beatOn = false, t0 = 0, nextN = 0;
let currentStep = -1, playingNow = 0, lastN = 0, shownVariation = null, chordNow = '';
let buildStartN = -1, buildEndN = -1, stutterOn = false, stutterStep = 0, cells = [];
const queue = [];                                          // scheduled steps, to update the screen
const EMPTY = '................';

// ---------- Helpers ----------
const hz = m => 440 * Math.pow(2, (m - 69) / 12);
const noteName = m => noteNames()[m % 12] + (Math.floor(m / 12) - 1);
const hue = i => Math.round(i * 360 / info.touchPins.length);   // same color as the LED
const stepDuration = () => 60 / bpm / 4;                   // one sixteenth note, in seconds
function stepOffset(n) {                                   // swing pushes the 'and' counts back a little
  const d = stepDuration();
  return n * d + (n % 2 === 1 ? swing / 100 * d * 0.66 : 0);
}
const stepTime = n => t0 + stepOffset(n);
function midiOf(i) {                                       // key 0 and 1 are drums, the notes start at 2
  const l = SCALES[scale], n = i - 2;
  return 57 + musicKey + 12 * Math.floor(n / l.length) + l[n % l.length];
}
function floatTimes() {
  const z = settings.float / 100;
  return {attack: 0.04 + z * 2.2, release: 0.5 + z * 5};
}
const filterHz = () => Math.min(250 * Math.pow(2, settings.bright / 100 * 4.6) * SOUNDS[sound].bright, 18000);
function chordOf(name) {                                   // chord in the chosen key
  const a = CHORDS[name], root = (a.root + musicKey + 12) % 12;
  return {name: noteNames()[root] + a.type, root, notes: a.notes.map(n => n + musicKey)};
}
const bassMidi = root => 33 + ((root - 9 + 12) % 12);      // bass between A1 and G#2
function makeReverb(seconds) {
  const n = Math.floor(ac.sampleRate * seconds), buf = ac.createBuffer(2, n, ac.sampleRate);
  for (let k = 0; k < 2; k++) {
    const d = buf.getChannelData(k);
    for (let i = 0; i < n; i++) d[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / n, 2.5);
  }
  return buf;
}
function makeNoise() {
  const n = ac.sampleRate, b = ac.createBuffer(1, n, ac.sampleRate), d = b.getChannelData(0);
  for (let i = 0; i < n; i++) d[i] = Math.random() * 2 - 1;
  return b;
}
const shapeCache = {};
function distortionCurve(k) {                              // soft distortion (tanh): warmer and easier to hear
  if (!shapeCache[k]) {
    const c = new Float32Array(1024);
    for (let i = 0; i < 1024; i++) { const x = i / 1023 * 2 - 1; c[i] = Math.tanh(x * (1 + k)) / Math.tanh(1 + k); }
    shapeCache[k] = c;
  }
  return shapeCache[k];
}
// Convert a moment (performance.now) to the audio clock, including the delay of your speaker
function audioTimeOf(moment) {
  if (ac.getOutputTimestamp) {
    const ts = ac.getOutputTimestamp();
    if (ts.performanceTime > 0) return ts.contextTime + (moment - ts.performanceTime) / 1000;
  }
  return ac.currentTime - (ac.outputLatency || ac.baseLatency || 0) - (performance.now() - moment) / 1000;
}

// ---------- Starting the sound: all channels and effects ----------
function startAudio() {
  if (ac) { if (ac.state !== 'running') ac.resume(); return; }
  ac = new (window.AudioContext || window.webkitAudioContext)();
  const comp = ac.createDynamicsCompressor();             // keeps things clean when everything sounds at once
  comp.threshold.value = -16; comp.knee.value = 14; comp.ratio.value = 3.5;
  comp.attack.value = 0.008; comp.release.value = 0.25;
  volumeNode = ac.createGain();
  comp.connect(volumeNode); volumeNode.connect(ac.destination);

  // Master: everything goes through the DJ filters and the build-up filter
  master = ac.createGain();
  djLP = ac.createBiquadFilter(); djLP.type = 'lowpass'; djLP.frequency.value = 20000; djLP.Q.value = 1.1;
  djHP = ac.createBiquadFilter(); djHP.type = 'highpass'; djHP.frequency.value = 10; djHP.Q.value = 1.1;
  buildHP = ac.createBiquadFilter(); buildHP.type = 'highpass'; buildHP.frequency.value = 10; buildHP.Q.value = 0.9;
  master.connect(djLP); djLP.connect(djHP); djHP.connect(buildHP); buildHP.connect(comp);

  // Synth channel with chorus; 'mix' is the input of the echo and reverb (also for the chords)
  synthBus = ac.createGain();
  mix = ac.createGain();
  const chorus = ac.createChannelMerger(2);
  synthBus.connect(mix);
  [0, 1].forEach(k => {
    const d = ac.createDelay(0.1), lfo = ac.createOscillator(), depth = ac.createGain();
    d.delayTime.value = 0.015 + k * 0.008;
    lfo.frequency.value = 0.21 + k * 0.13;
    depth.gain.value = 0.0035;
    lfo.connect(depth); depth.connect(d.delayTime); lfo.start();
    synthBus.connect(d); d.connect(chorus, 0, k);
  });
  chorus.connect(mix);
  const dry = ac.createGain();
  dry.gain.value = 0.55;
  mix.connect(dry); dry.connect(master);

  // Echo that runs in time with the beat
  const echo = ac.createDelay(2), feedback = ac.createGain(), echoTone = ac.createBiquadFilter();
  echo.delayTime.value = 0.43; feedback.gain.value = 0.48;
  echoTone.type = 'lowpass'; echoTone.frequency.value = 1800;
  echoNode = echo;
  echoLevel = ac.createGain();
  mix.connect(echo); echo.connect(echoTone); echoTone.connect(feedback); feedback.connect(echo);
  echoTone.connect(echoLevel); echoLevel.connect(master);

  // Reverb: as if you are standing in a huge hall
  reverb = ac.createConvolver();
  reverb.buffer = makeReverb(5);
  reverbLevel = ac.createGain();
  mix.connect(reverb); echoLevel.connect(reverb); reverb.connect(reverbLevel); reverbLevel.connect(master);

  // Drums and bass go dry to the master; the chords go through the echo and reverb
  noise = makeNoise();
  drumBus = ac.createGain(); drumBus.connect(master);
  drumReverb = ac.createGain(); drumReverb.gain.value = 0.2; drumReverb.connect(reverb);
  fxBus = ac.createGain(); fxBus.gain.value = 0.8; fxBus.connect(master);
  trackGain = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9].map(s => { const g = ac.createGain(); g.connect(s === 9 ? mix : drumBus); return g; });

  // Slow 'breathing': the sound of the synth waves along very gently
  const breath = ac.createOscillator();
  breath.frequency.value = 0.07;
  breathDepth = ac.createGain();
  breath.connect(breathDepth); breath.start();

  applySettings();
  setDjFilter(+$('d-filter').value);
}

function applySettings() {
  if (!ac) return;
  const t = ac.currentTime;
  volumeNode.gain.setTargetAtTime(settings.master / 100, t, 0.05);
  synthBus.gain.setTargetAtTime(synthMute ? 0 : settings.synth / 100 * 0.8, t, 0.05);
  reverbLevel.gain.setTargetAtTime(settings.reverb / 100 * 1.4, t, 0.1);
  echoLevel.gain.setTargetAtTime(settings.echo / 100 * 0.9, t, 0.1);
  breathDepth.gain.setTargetAtTime(filterHz() * 0.3, t, 0.1);
  echoNode.delayTime.setTargetAtTime(stepDuration() * 3, t, 0.1);
  trackGain.forEach((g, s) => g.gain.setTargetAtTime(trackOff[s] ? 0 : (s === 9 ? 1.6 : 1), t, 0.015));
  Object.values(voices).forEach(st => st.filter.frequency.setTargetAtTime(filterHz(), t, 0.2));
}

function setDjFilter(v) {                                  // -100 (dull) .. 0 (off) .. 100 (thin)
  $('d-filterv').textContent = v === 0 ? 'off' : (v < 0 ? 'dull ' + (-v) : 'thin ' + v) + '%';
  if (!ac) return;
  const t = ac.currentTime;
  djLP.frequency.setTargetAtTime(v < 0 ? 20000 * Math.pow(2, v / 100 * 8) : 20000, t, 0.03);
  djHP.frequency.setTargetAtTime(v > 0 ? 10 * Math.pow(2, v / 100 * 10) : 10, t, 0.03);
}

// ---------- Drum sounds: all built by ourselves from waves and noise ----------
function noiseSource(t, length) {
  const r = ac.createBufferSource();
  r.buffer = noise;
  if (length > 0.4) r.loop = true;
  r.start(t, Math.random() * 0.5);
  r.stop(t + length);
  return r;
}
function click(t, out, v) {
  const f = ac.createBiquadFilter(), k = ac.createGain();
  f.type = 'highpass'; f.frequency.value = 3000;
  k.gain.setValueAtTime(v, t);
  k.gain.exponentialRampToValueAtTime(0.0001, t + 0.015);
  noiseSource(t, 0.03).connect(f); f.connect(k); k.connect(out);
}
function kickSound(t, out, v, p) {
  const o = ac.createOscillator(), g = ac.createGain();
  o.frequency.setValueAtTime(p.kickStart, t);
  o.frequency.exponentialRampToValueAtTime(p.kickEnd, t + 0.11);   // the pitch drops fast: that is the 'boom'
  g.gain.setValueAtTime(0.0001, t);
  g.gain.exponentialRampToValueAtTime(v, t + 0.004);
  g.gain.exponentialRampToValueAtTime(0.0001, t + p.kickLength);
  o.connect(g); g.connect(out);
  o.start(t); o.stop(t + p.kickLength + 0.05);
  click(t, out, 0.25 * v * p.kickClick);
}
function clapSound(t, out, v) {
  const f = ac.createBiquadFilter(), g = ac.createGain();
  f.type = 'bandpass'; f.frequency.value = 1500; f.Q.value = 0.9;
  g.gain.setValueAtTime(0.0001, t);
  [0, 0.012, 0.024].forEach(d => {                        // three quick claps in a row = one clap
    g.gain.setValueAtTime(0.7 * v, t + d);
    g.gain.exponentialRampToValueAtTime(0.15 * v, t + d + 0.01);
  });
  g.gain.setValueAtTime(0.5 * v, t + 0.036);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.3);
  noiseSource(t, 0.35).connect(f); f.connect(g); g.connect(out); g.connect(drumReverb);
}
function rimSound(t, out, v) {
  const o = ac.createOscillator(), g = ac.createGain();
  o.type = 'triangle'; o.frequency.value = 1750;
  g.gain.setValueAtTime(0.35 * v, t);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.03);
  o.connect(g); g.connect(out);
  o.start(t); o.stop(t + 0.04);
  click(t, out, 0.2 * v);
}
function claveSound(t, out, v) {                           // wooden tick
  const o = ac.createOscillator(), g = ac.createGain();
  o.frequency.value = 2500;
  g.gain.setValueAtTime(0.0001, t);
  g.gain.exponentialRampToValueAtTime(0.4 * v, t + 0.001);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.06);
  o.connect(g); g.connect(out);
  o.start(t); o.stop(t + 0.07);
}
let prevOpen = null;
function hihatSound(t, out, v) {
  if (prevOpen && t >= prevOpen.start) prevOpen.gain.setTargetAtTime(0, t, 0.005);   // a closed hihat chokes the open hihat
  const f = ac.createBiquadFilter(), g = ac.createGain();
  f.type = 'highpass'; f.frequency.value = 7000;
  g.gain.setValueAtTime(0.0001, t);
  g.gain.exponentialRampToValueAtTime(0.3 * v, t + 0.002);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.045);
  noiseSource(t, 0.06).connect(f); f.connect(g); g.connect(out);
}
function openSound(t, out, v) {
  const f = ac.createBiquadFilter(), g = ac.createGain(), cut = ac.createGain();
  f.type = 'highpass'; f.frequency.value = 6500;
  g.gain.setValueAtTime(0.0001, t);
  g.gain.exponentialRampToValueAtTime(0.25 * v, t + 0.004);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.32);
  noiseSource(t, 0.35).connect(f); f.connect(g); g.connect(cut); cut.connect(out);
  cut.start = t;
  if (!prevOpen || t >= prevOpen.start) prevOpen = cut;
}
function shakerSound(t, out, v) {
  const f = ac.createBiquadFilter(), g = ac.createGain();
  f.type = 'bandpass'; f.frequency.value = 5500; f.Q.value = 1.2;
  g.gain.setValueAtTime(0.0001, t);
  g.gain.linearRampToValueAtTime(0.22 * v, t + 0.012);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.08);
  noiseSource(t, 0.1).connect(f); f.connect(g); g.connect(out);
}
function rideSound(t, out, v) {                            // cymbal: metallic hiss with a long tail
  const f = ac.createBiquadFilter(), g = ac.createGain();
  f.type = 'highpass'; f.frequency.value = 5000;
  g.gain.setValueAtTime(0.0001, t);
  g.gain.exponentialRampToValueAtTime(0.12 * v, t + 0.003);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.7);
  noiseSource(t, 0.75).connect(f); f.connect(g); g.connect(out);
  [3150, 4870, 6230].forEach(fr => {                       // the metal 'ping'
    const o = ac.createOscillator(), go = ac.createGain();
    o.type = 'square'; o.frequency.value = fr;
    go.gain.setValueAtTime(0.012 * v, t);
    go.gain.exponentialRampToValueAtTime(0.0001, t + 0.5);
    o.connect(go); go.connect(f);
    o.start(t); o.stop(t + 0.55);
  });
}
let congaCount = 0;
function congaSound(t, out, v) {
  const o = ac.createOscillator(), g = ac.createGain(), high = congaCount++ % 2 === 0;   // alternating high and low
  o.frequency.setValueAtTime(high ? 360 : 260, t);
  o.frequency.exponentialRampToValueAtTime(high ? 300 : 210, t + 0.05);
  g.gain.setValueAtTime(0.0001, t);
  g.gain.exponentialRampToValueAtTime(0.45 * v, t + 0.004);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.22);
  o.connect(g); g.connect(out);
  o.start(t); o.stop(t + 0.25);
}
let bongoCount = 0;
function bongoSound(t, out, v) {
  const o = ac.createOscillator(), g = ac.createGain(), high = bongoCount++ % 3 !== 2;
  o.frequency.setValueAtTime(high ? 620 : 470, t);
  o.frequency.exponentialRampToValueAtTime(high ? 520 : 400, t + 0.04);
  g.gain.setValueAtTime(0.0001, t);
  g.gain.exponentialRampToValueAtTime(0.35 * v, t + 0.002);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.12);
  o.connect(g); g.connect(out);
  o.start(t); o.stop(t + 0.14);
}
const INSTR = {kick: kickSound, clap: clapSound, rim: rimSound, clave: claveSound, hihat: hihatSound,
               open: openSound, shaker: shakerSound, ride: rideSound, conga: congaSound, bongo: bongoSound};

// ---------- Bass and chords ----------
let prevBass = null;
function bassSound(t, out, v, note, length) {
  if (prevBass && t >= prevBass.start) prevBass.gain.setTargetAtTime(0, t, 0.008);   // always only one bass note at a time
  const f = hz(note), o = ac.createOscillator(), o2 = ac.createOscillator(), g2 = ac.createGain();
  const filter = ac.createBiquadFilter(), shaper = ac.createWaveShaper(), env = ac.createGain(), cut = ac.createGain();
  o.type = 'sine'; o.frequency.value = f;
  o2.type = 'triangle'; o2.frequency.value = f * 2;       // overtone: this way you can hear the bass on a phone too
  g2.gain.value = 0.22;
  filter.type = 'lowpass'; filter.Q.value = 2;
  filter.frequency.setValueAtTime(1200, t);
  filter.frequency.exponentialRampToValueAtTime(280, t + 0.18);
  shaper.curve = distortionCurve(1.2);
  env.gain.setValueAtTime(0.0001, t);
  env.gain.exponentialRampToValueAtTime(0.85 * v, t + 0.006);
  env.gain.setTargetAtTime(0.5 * v, t + 0.006, 0.08);
  env.gain.setTargetAtTime(0.0001, t + length, 0.04);
  o.connect(filter); o2.connect(g2); g2.connect(filter);
  filter.connect(shaper); shaper.connect(env); env.connect(cut); cut.connect(out);
  o.start(t); o2.start(t);
  o.stop(t + length + 0.3); o2.stop(t + length + 0.3);
  cut.start = t;
  if (!prevBass || t >= prevBass.start) prevBass = cut;    // a preview note must not choke a scheduled note
}
function chordSound(t, out, v, notes) {
  if (set.chordStyle === 'kalimba') {                      // afro: plucked, like a kalimba
    notes.forEach((n, k) => {
      const tk = t + k * 0.018, f = hz(n + 12);
      const o = ac.createOscillator(), o3 = ac.createOscillator(), g = ac.createGain(), g3 = ac.createGain();
      o.frequency.value = f;
      o3.frequency.value = f * 3.98;
      g3.gain.value = 0.12;
      g.gain.setValueAtTime(0.0001, tk);
      g.gain.exponentialRampToValueAtTime(0.16 * v, tk + 0.003);
      g.gain.exponentialRampToValueAtTime(0.0001, tk + 0.9);
      o.connect(g); o3.connect(g3); g3.connect(g); g.connect(out);
      o.start(tk); o3.start(tk); o.stop(tk + 1); o3.stop(tk + 1);
    });
    return;
  }
  // deep house: a short, warm stab that fades away in the echo and the reverb
  const filter = ac.createBiquadFilter(), g = ac.createGain();
  filter.type = 'lowpass'; filter.Q.value = 1.5;
  filter.frequency.setValueAtTime(2600, t);
  filter.frequency.exponentialRampToValueAtTime(700, t + 0.25);
  g.gain.setValueAtTime(0.0001, t);
  g.gain.exponentialRampToValueAtTime(0.09 * v, t + 0.004);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.55);
  filter.connect(g); g.connect(out);
  notes.forEach(n => {
    [['sawtooth', -7], ['square', 7]].forEach(([wave, cent]) => {
      const o = ac.createOscillator();
      o.type = wave; o.frequency.value = hz(n); o.detune.value = cent;
      o.connect(filter);
      o.start(t); o.stop(t + 0.6);
    });
  });
}

function impact(t) {                                       // the big hit at the drop
  const o = ac.createOscillator(), g = ac.createGain(), f = ac.createBiquadFilter(), c = ac.createGain();
  o.frequency.setValueAtTime(70, t);
  o.frequency.exponentialRampToValueAtTime(30, t + 1);
  g.gain.setValueAtTime(0.9, t);
  g.gain.exponentialRampToValueAtTime(0.0001, t + 1.2);
  o.connect(g); g.connect(fxBus);
  o.start(t); o.stop(t + 1.3);
  f.type = 'highpass'; f.frequency.value = 3500;
  c.gain.setValueAtTime(0.35, t);
  c.gain.exponentialRampToValueAtTime(0.0001, t + 1.6);
  noiseSource(t, 1.7).connect(f); f.connect(c); c.connect(fxBus); c.connect(drumReverb);
}
function riser(tS, tE) {                                   // noise that swells up during the build-up
  const r = ac.createBufferSource(), f = ac.createBiquadFilter(), g = ac.createGain();
  r.buffer = noise; r.loop = true;
  f.type = 'bandpass'; f.Q.value = 1.5;
  f.frequency.setValueAtTime(300, tS);
  f.frequency.exponentialRampToValueAtTime(7000, tE);
  g.gain.setValueAtTime(0.0001, tS);
  g.gain.exponentialRampToValueAtTime(0.35, tE - 0.02);
  g.gain.setValueAtTime(0.0001, tE);
  r.connect(f); f.connect(g); g.connect(fxBus);
  r.start(tS); r.stop(tE + 0.05);
  riserSource = r; riserGain = g; riserFilter = f; riserStart = tS;
}

// Play one symbol of a drum track: x = hit, o = soft, 2/3/4 = roll within one step
function playSymbol(kind, s, symbol, t, d, mult) {
  if (symbol === '.') return;
  const v = (symbol === 'o' ? 0.45 : 1) * mult, count = '234'.includes(symbol) ? +symbol : 1;
  for (let k = 0; k < count; k++) INSTR[kind](t + k * d / count, trackGain[s], v * (1 - k * 0.12), set.drumSound);
}
function playMelody(kind, s, symbol, step, row, t, d) {
  if (!barChord) return;
  if (kind === 'bass') {
    let length = 1;                                        // the note sounds until the next bass note (max 4 steps)
    while (length < 4 && step + length < 16 && row[step + length] === '.') length++;
    bassSound(t, trackGain[s], symbol === 'o' ? 0.75 : 1, bassMidi(barChord.root) + (symbol === 'o' ? 12 : 0), length * d * 0.92);
  } else {
    chordSound(t, trackGain[s], symbol === 'o' ? 0.55 : 1, barChord.notes);
  }
}

// ---------- Random: small variations and new patterns ----------
function vary(pat, wild) {
  const chance = 0.02 + 0.12 * wild;                    // wildness 40% = about 9 small changes per bar
  return pat.map((row, s) => {
    const rule = kit.variation[s];
    if (rule === 'fixed') return row;
    const r = row.split('');
    for (let i = 0; i < 16; i++) {
      if (Math.random() > chance) continue;
      const t = r[i];
      if (rule === 'light') {                              // clap: only add or remove soft ticks
        if (t === '.' && i % 4 !== 0) r[i] = 'o';
        else if (t === 'o') r[i] = '.';
      } else if (rule === 'free') {                        // hats and percussion: anything goes
        if (t === '.') r[i] = Math.random() < 0.7 ? 'o' : 'x';
        else r[i] = Math.random() < 0.5 ? '.' : (t === 'x' ? 'o' : 'x');
        if (kit.rollTracks.includes(s) && r[i] !== '.' && Math.random() < 0.25) r[i] = '2';
      } else if (t !== '.') {                              // bass and chord: move one step, or switch octave
        if (i < 15 && r[i + 1] === '.' && Math.random() < 0.5) { r[i + 1] = t; r[i] = '.'; i++; }
        else r[i] = t === 'x' ? 'o' : 'x';
      } else if (i % 2 === 0 && Math.random() < 0.3) {
        r[i] = 'o';
      }
    }
    return r.join('');
  });
}
function generatePattern() {                               // dice: pick a rhythm from the set for each track
  const pick = list => list[Math.floor(Math.random() * list.length)];
  return vary(set.templates.map(pick), 0.25);
}
function pickRandomPattern() {
  const candidates = [];
  for (let p = 0; p < 4; p++) {
    if (p === playPattern) continue;
    candidates.push(p);
    if (p !== set.breakPattern) candidates.push(p);        // a break comes less often
  }
  return candidates[Math.floor(Math.random() * candidates.length)];
}

// ---------- The scheduler: looks ahead and puts everything exactly on the beat ----------
function newBar(n) {                                       // at the start of every bar
  const building = buildStartN >= 0 && n >= buildStartN && n < buildEndN;
  if (buildEndN >= 0 && n === buildEndN) {
    playPattern = set.peakPattern;                         // the drop lands on the fullest pattern
    nextPattern = -1;
  } else if (nextPattern >= 0) {
    playPattern = nextPattern;
    nextPattern = -1;
  } else if (randomOn && barCount > 0 && !building) {
    const w = wildness / 100, chance = barCount % 4 === 0 ? 0.25 + 0.55 * w : 0.08 * w;
    if (Math.random() < chance) playPattern = pickRandomPattern();
  }
  const prog = set.progressions[progression];
  barChord = chordOf(prog[barCount % prog.length]);
  fillThisBar = fillNext || (fillsOn && barCount % 4 === 3);
  fillNext = false;
  barPattern = randomOn ? vary(patterns[playPattern], wildness / 100) : patterns[playPattern];
  barCount++;
}
function buildSymbol(pos) {                                // drum roll that keeps getting faster
  if (pos < 16) return pos % 4 === 0 ? 'x' : '.';
  if (pos < 24) return pos % 2 === 0 ? 'x' : '.';
  return pos < 28 ? 'x' : '2';
}
function playStep(n, step, t) {
  const pat = barPattern, d = stepDuration(), src = stutterOn ? stutterStep : step;
  const build = buildStartN >= 0 && n >= buildStartN && n < buildEndN, pos = n - buildStartN;
  kit.tracks.forEach(([kind], s) => {
    if (trackOff[s]) return;
    let symbol = pat[s][src], mult = 1;
    if (fillThisBar && !stutterOn && src >= 12 && kit.fill[s] !== undefined) symbol = kit.fill[s][src - 12];
    if (build) {
      if (pos >= 24 && (s === 0 || s === 8)) symbol = '.'; // last half bar: no kick and bass
      if (s === kit.buildTrack) { symbol = buildSymbol(pos); mult = 0.35 + 0.65 * pos / 32; }
    }
    if (symbol === '.') return;
    if (kind === 'bass' || kind === 'chord') {
      if (!stutterOn || step % 2 === 0) playMelody(kind, s, symbol, src, pat[s], t, d);
    } else {
      playSymbol(kind, s, stutterOn ? '2' : symbol, t, d, mult);
      if (kind === 'kick' || kind === 'clap') beatFlash(kind === 'kick' ? 165 : 320, t, symbol === 'o' ? 0.5 : mult);
    }
  });
  if (n === buildEndN) { impact(t); buildStartN = -1; buildEndN = -1; }   // the drop!
}
// Make the LED flash along: we tell the ESP32 in advance in how many ms the kick or clap will sound.
// For that we add the delay of your speaker and subtract half the round trip time of the message.
function beatFlash(ledHue, t, strength) {
  if (!liveOpen || !live || !$('m-ledbeat').checked) return;
  const latency = ac.outputLatency || ac.baseLatency || 0;
  const ms = Math.round((t - ac.currentTime + latency) * 1000 - roundTrip / 2);
  if (ms >= 0 && ms < 3000) {
    try { live.send('f' + ms + ',' + ledHue + ',' + Math.round(Math.min(1, strength) * 255)); } catch (e) {}
  }
}
function scheduler() {
  if (!beatOn || !ac) return;
  const ahead = document.hidden ? 1.6 : 0.12;              // screen off? then plan further ahead
  while (stepTime(nextN) < ac.currentTime + ahead) {
    const n = nextN, step = n % 16, t = stepTime(n);
    if (step === 0) newBar(n);
    playStep(n, step, t);
    queue.push({step, t, n, pattern: playPattern, variation: barPattern, chord: barChord ? barChord.name : ''});
    nextN++;
  }
}
setInterval(scheduler, 25);

// ---------- Starting, stopping and recording drums ----------
function beatStart(start) {
  if (!ac) startAudioButton();
  beatOn = true;
  queue.length = 0;
  barCount = 0;
  fillNext = false;
  playPattern = editPattern;
  nextPattern = -1;
  const now = ac.currentTime;
  if (start !== undefined && start < now) {
    // Started with a drum tap: count 1 lies at the moment you really touched the pin
    t0 = start;
    newBar(0);
    playStep(0, 0, now);
    queue.push({step: 0, t: now, n: 0, pattern: playPattern, variation: barPattern, chord: barChord.name});
    nextN = Math.max(1, Math.ceil((now - t0) / stepDuration()));
  } else {
    t0 = now + 0.05;
    nextN = 0;
  }
  drawAll();
}
function beatStop() {
  beatOn = false;
  currentStep = -1;
  queue.length = 0;
  stopBuild();
  drawAll();
}
function beatToggle() { if (beatOn) beatStop(); else beatStart(); }

function setSymbol(p, s, step, symbol) {                   // also changes right away what plays in this bar
  const r = patterns[p][s], updated = r.slice(0, step) + symbol + r.slice(step + 1);
  if (beatOn && p === playPattern && barPattern && barPattern !== patterns[p]) {
    const m = barPattern[s];
    barPattern[s] = m.slice(0, step) + symbol + m.slice(step + 1);
  }
  patterns[p][s] = updated;
}
// Drum touched (pin 1 or 2) at moment 'tap': play it and record it on the nearest step
function drumTap(i, tap) {
  if (!ac) return;
  const s = kit.pinTracks[i], kind = kit.tracks[s][0];
  if (!beatOn) {                                           // the first tap starts the beat, on count 1
    if (patterns[editPattern].every(r => r === EMPTY)) patterns[editPattern][kit.rollTracks[0]] = kit.emptyHihat;
    setSymbol(editPattern, s, 0, 'x');
    beatStart(tap);
    return;
  }
  const d = stepDuration(), n = Math.round((tap - t0) / d), step = ((n % 16) + 16) % 16, tStep = stepTime(n);
  if (patterns[playPattern][s][step] !== '.') return;      // already there: it plays by itself
  setSymbol(playPattern, s, step, 'x');
  drawGrid();
  const now = ac.currentTime;
  if (tStep <= now) playSymbol(kind, s, 'x', now, d, 1);   // just too late for the count: play it right away
  else if (n < nextN) playSymbol(kind, s, 'x', tStep, d, 1);   // step was already scheduled: add it anyway
}

// ---------- Build-up, drop and stutter ----------
function startBuild() {
  if (!beatOn || buildStartN >= 0) return;
  buildStartN = Math.ceil(nextN / 16) * 16;                // starts at the next bar
  buildEndN = buildStartN + 32;                            // lasts 2 bars, then the drop
  riserSource = null;                                      // the riser of a previous build-up is already done
  planBuildSound();
  drawStatus();
}
// Put the filter and riser exactly on the build-up (again if you change the tempo during the build-up)
function planBuildSound() {
  const now = ac.currentTime, tS0 = stepTime(buildStartN), tS = Math.max(now, tS0), tE = stepTime(buildEndN), f = buildHP.frequency;
  holdValue(f, now);
  if (tS0 > now) f.setValueAtTime(10, tS0);                // the build-up only starts at the next bar
  if (tE - 0.03 > tS) f.exponentialRampToValueAtTime(1500, tE - 0.03);   // everything gets thinner and thinner...
  f.setValueAtTime(10, tE);                                // ...and at the drop everything is full again
  if (riserSource && riserStart < now && tE - 0.05 > now) {
    // The riser is already running: do not start it again (that gives a click), but let it keep swelling to the new end
    holdValue(riserGain.gain, now);
    holdValue(riserFilter.frequency, now);
    riserGain.gain.exponentialRampToValueAtTime(0.35, tE - 0.02);
    riserGain.gain.setValueAtTime(0.0001, tE);
    riserFilter.frequency.exponentialRampToValueAtTime(7000, tE);
    riserSource.stop(tE + 0.05);
  } else {
    if (riserSource) { try { riserSource.stop(); } catch (e) {} }
    if (tE - 0.05 > tS) riser(tS, tE);
  }
}
// Stop a scheduled change and hold the value of this moment
function holdValue(p, t) {
  if (p.cancelAndHoldAtTime) p.cancelAndHoldAtTime(t);
  else { p.cancelScheduledValues(t); p.setValueAtTime(Math.max(0.0001, p.value), t); }
}
function stopBuild() {
  buildStartN = -1;
  buildEndN = -1;
  if (!ac) return;
  buildHP.frequency.cancelScheduledValues(0);
  buildHP.frequency.setValueAtTime(10, ac.currentTime);
  if (riserSource) { try { riserSource.stop(); } catch (e) {} riserSource = null; }
}

// ---------- Updating the screen ----------
function buildGrid() {
  const r = $('b-grid');
  r.innerHTML = '';
  cells = [];
  kit.tracks.forEach(([kind, name], s) => {
    const label = document.createElement('button');
    label.className = 'rowlabel';
    label.textContent = name;
    label.onclick = () => {                                // tap the name: hear the sound
      startAudioButton();
      const t = ac.currentTime + 0.01;
      if (kind === 'bass' || kind === 'chord') {
        if (!barChord) barChord = chordOf(set.progressions[progression][0]);
        playMelody(kind, s, 'x', 0, 'x...............', t, stepDuration());
      } else playSymbol(kind, s, 'x', t, stepDuration(), 1);
    };
    r.appendChild(label);
    cells[s] = [];
    for (let i = 0; i < 16; i++) {
      const c = document.createElement('button');
      c.onclick = () => {
        const order = kit.rollTracks.includes(s) ? '.x234' : '.xo';   // tap order
        const cur = patterns[editPattern][s][i];
        setSymbol(editPattern, s, i, order[(order.indexOf(cur) + 1) % order.length]);
        drawGrid();
      };
      cells[s][i] = c;
      r.appendChild(c);
    }
  });
  drawGrid();
}
function drawGrid() {
  const pat = patterns[editPattern], isLive = beatOn && playingNow === editPattern;
  const varPat = isLive && randomOn ? shownVariation : null;
  cells.forEach((row, s) => row.forEach((c, i) => {
    const t = pat[s][i], roll = '234'.includes(t), v = varPat ? varPat[s][i] : t;
    let cls = 'cell' + (i % 4 === 0 ? ' count' : '') + (s >= 8 ? ' melody' : '');
    cls += t === 'x' || roll ? ' full' : t === 'o' ? ' soft' : '';
    if (varPat && v !== t) cls += v === '.' ? ' gone' : ' extra';   // what random changes this bar
    if (isLive && i === currentStep) cls += ' now';
    c.className = cls;
    c.textContent = roll ? t : '';
  }));
}
function buildPatternButtons() {
  const b = $('b-patterns');
  b.innerHTML = '';
  set.names.forEach((name, p) => {
    const btn = document.createElement('button');
    btn.innerHTML = 'ABCD'[p] + '<small>' + name + '</small>';
    btn.onclick = () => {
      editPattern = p;
      if (beatOn) nextPattern = p === playPattern ? -1 : p;   // switches on the next bar
      else { playPattern = p; playingNow = p; }
      drawAll();
    };
    b.appendChild(btn);
  });
}
function drawPatternButtons() {
  [...$('b-patterns').children].forEach((btn, p) => {
    btn.classList.toggle('editing', p === editPattern);
    btn.classList.toggle('playing', beatOn && p === playingNow);
    btn.classList.toggle('waiting', p === nextPattern);
  });
}
function buildPads() {                                     // the big Live buttons
  const l = $('l-pads');
  l.innerHTML = '';
  Object.keys(kit.groups).concat(['Synth']).forEach(name => {
    const btn = document.createElement('button');
    btn.textContent = name;
    btn.onclick = () => {
      if (name === 'Synth') synthMute = !synthMute;
      else groupOff[name] = !groupOff[name];
      trackOff = kit.tracks.map((_, s) => Object.keys(kit.groups).some(g => groupOff[g] && kit.groups[g].includes(s)));
      applySettings();
      drawPads();
    };
    l.appendChild(btn);
  });
  drawPads();
}
function drawPads() {
  [...$('l-pads').children].forEach(btn => {
    const off = btn.textContent === 'Synth' ? synthMute : !!groupOff[btn.textContent];
    btn.classList.toggle('on', !off);
  });
}
function drawStatus() {
  const btn = $('b-start'), st = $('b-status');
  btn.textContent = beatOn ? 'Stop' : 'Start';
  btn.classList.toggle('stop', beatOn);
  let text = 'off';
  if (beatOn) {
    text = 'ABCD'[playingNow] + ' ' + set.names[playingNow] + (chordNow ? ' \u00b7 ' + chordNow : '');
    if (buildStartN >= 0) {
      const bars = Math.max(1, Math.ceil((buildEndN - lastN) / 16));
      text += ' \u00b7 drop in ' + bars + (bars === 1 ? ' bar' : ' bars');
    }
  }
  st.textContent = text;
  st.classList.toggle('on', beatOn);
  $('d-build').classList.toggle('on', buildStartN >= 0);
  $('r-toggle').textContent = 'Random ' + (randomOn ? 'on' : 'off');
  $('r-toggle').classList.toggle('on', randomOn);
}
function drawAll() { drawGrid(); drawPatternButtons(); drawStatus(); }

function animate() {                                       // let the screen follow along with what you hear
  if (ac && beatOn) {
    let changed = false;
    while (queue.length && queue[0].t <= ac.currentTime) {
      const e = queue.shift();
      currentStep = e.step; playingNow = e.pattern; lastN = e.n; shownVariation = e.variation; chordNow = e.chord;
      changed = true;
    }
    if (changed) drawAll();
  }
  requestAnimationFrame(animate);
}

// ---------- Loading a set ----------
function loadSet(name) {
  const wasPlaying = beatOn;
  if (beatOn) beatStop();
  setName = name; set = SETS[name]; kit = KITS[set.kit];
  patterns = set.patterns.map(p => p.slice());
  editPattern = 0; playPattern = 0; playingNow = 0; nextPattern = -1;
  bpm = set.bpm; swing = set.swing; scale = set.scale; sound = set.sound;
  groupOff = {}; trackOff = kit.tracks.map(() => false); synthMute = false;
  $('b-tempo').value = bpm; $('b-bpm').textContent = bpm;
  $('b-swing').value = swing; $('b-swingv').textContent = swing + '%';
  $('z-scale').value = scale;
  $('z-preset').value = sound;
  $('m-setinfo').textContent = set.info;
  $('c-prog').innerHTML = '';
  fillSelect('c-prog', Object.keys(set.progressions));
  progression = Object.keys(set.progressions)[0];
  barChord = chordOf(set.progressions[progression][0]);
  buildPatternButtons(); buildGrid(); buildPads(); setNoteNames(); drawAll(); applySettings(); fillSlots();
  if (wasPlaying) beatStart();
}

// ---------- Synth on the touch pins ----------
function noteOn(i) {
  if (!ac || voices[i]) return;
  const p = SOUNDS[sound], t = ac.currentTime, f = hz(midiOf(i)), {attack} = floatTimes(), cutoff = filterHz();
  const volume = ac.createGain(), filter = ac.createBiquadFilter();
  const pan = ac.createStereoPanner ? ac.createStereoPanner() : ac.createGain();
  if (pan.pan) pan.pan.value = (i - 2) / 11 * 1.4 - 0.7;  // low notes left, high notes right
  volume.gain.setValueAtTime(0, t);
  if (p.pluck) {                                           // bell: loud right away, then dies out by itself
    volume.gain.linearRampToValueAtTime(0.22, t + 0.005);
    volume.gain.setTargetAtTime(0.0001, t + 0.005, p.pluck / 5);
  } else {
    volume.gain.linearRampToValueAtTime(0.16, t + attack * p.attack);
  }
  filter.type = 'lowpass';
  filter.Q.value = 0.7;
  filter.frequency.setValueAtTime(cutoff * 0.3, t);
  filter.frequency.setTargetAtTime(cutoff, t, (p.pluck ? 0.01 : attack * p.attack * 0.6) + 0.02);
  breathDepth.connect(filter.frequency);
  filter.connect(volume); volume.connect(pan); pan.connect(synthBus);
  // One voice = a few oscillators that are tuned just slightly apart: that sounds full and wide
  const sources = [], vib = ac.createOscillator(), vibDepth = ac.createGain();
  vib.frequency.value = 0.3 + Math.random() * 0.3;         // very slow vibrato
  vibDepth.gain.value = 5;
  vib.connect(vibDepth);
  p.layers.forEach(([mult, wave, cent, level]) => {
    const o = ac.createOscillator(), g = ac.createGain();
    o.type = wave;
    o.frequency.value = f * mult;
    o.detune.value = cent;
    g.gain.value = level;
    vibDepth.connect(o.detune);
    o.connect(g); g.connect(filter);
    o.start(t);
    sources.push(o);
  });
  vib.start(t);
  sources.push(vib);
  voices[i] = {volume, filter, sources};
}
function noteOff(i) {
  const s = voices[i];
  if (!s) return;
  delete voices[i];
  const t = ac.currentTime, release = floatTimes().release * (SOUNDS[sound].release || 1), g = s.volume.gain;
  if (g.cancelAndHoldAtTime) g.cancelAndHoldAtTime(t);
  else { g.cancelScheduledValues(t); g.setValueAtTime(g.value, t); }
  g.setTargetAtTime(0, t, release / 4);                    // fade out slowly
  s.sources.forEach(o => o.stop(t + release * 1.3));
  setTimeout(() => { try { breathDepth.disconnect(s.filter.frequency); } catch (e) {} }, release * 1400);
}
// Should key i sound? Yes if the pin is being touched or held on the screen
function updateVoice(i) {
  if (i < 2) {                                             // drums: only light up
    if (pinKeys[i]) pinKeys[i].classList.toggle('on', !!(espOn[i] || screenOn[i]));
    return;
  }
  const should = espOn[i] || screenOn[i] || Date.now() < tapUntil[i];
  if (should && !voices[i]) noteOn(i);
  if (!should && voices[i]) noteOff(i);
  if (pinKeys[i]) pinKeys[i].classList.toggle('on', !!should);
}
// Only call this from a tap or click: an iPhone only turns sound on when you touch something
// Connection lost? Then we no longer know which pins you are touching: release everything from the ESP32
function releaseAll() {
  espOn = espOn.map(() => false);
  for (let i = 0; i < espOn.length; i++) updateVoice(i);
}
function startAudioButton() {
  startAudio();
  const btn = $('z-sound');
  const show = () => {
    const on = ac.state === 'running';
    btn.disabled = on;
    btn.textContent = on ? 'Sound is on' : 'Tap here to turn on the sound';
  };
  ac.onstatechange = show;                                 // e.g. after a phone call or a locked screen
  show();
}
const audioReady = () => ac && ac.state === 'running';
function screenPress(i, on) {
  if (on && !ac) startAudioButton();
  if (i < 2 && on && !screenOn[i]) drumTap(i, audioTimeOf(performance.now()));   // drum: only when pressed down
  screenOn[i] = on;
  updateVoice(i);
}
function attachKey(btn, i) {
  btn.addEventListener('pointerdown', e => {
    e.preventDefault();
    try { btn.releasePointerCapture(e.pointerId); } catch (x) {}   // so you can swipe across the keys
    startAudioButton();                                    // a real tap: now the sound may turn on
    screenPress(i, true);
  });
  btn.addEventListener('pointerenter', e => { if (e.buttons > 0) screenPress(i, true); });
  ['pointerleave', 'pointerup', 'pointercancel'].forEach(type => btn.addEventListener(type, () => screenPress(i, false)));
}
function setNoteNames() {
  noteLabels.forEach((n, i) => { if (n) n.textContent = i < 2 ? kit.tracks[kit.pinTracks[i]][1] : noteName(midiOf(i)); });
}
function buildBoard() {
  const board = $('z-board'), pins = info.touchPins;
  board.innerHTML = '';
  espOn = pins.map(() => false);
  screenOn = pins.map(() => false);
  tapUntil = pins.map(() => 0);
  const pcb = document.createElement('div');
  pcb.className = 'pcb';
  pcb.innerHTML = '<div class="module">ESP32-S3</div><div>USB<div class="usb"></div><div class="usb"></div></div>';
  board.appendChild(pcb);
  [[ROW_LEFT, 1, 'left'], [ROW_RIGHT, 3, 'right']].forEach(([row, col, side]) => {
    row.forEach((name, r) => {
      const slot = document.createElement('div');
      slot.className = 'pinslot ' + side;
      slot.style.gridColumn = col;
      slot.style.gridRow = r + 1;
      const gpio = parseInt(name, 10), i = String(gpio) === name ? pins.indexOf(gpio) : -1;
      if (i >= 0) {
        const btn = document.createElement('button'), n = document.createElement('span');
        btn.className = 'pinkey';
        btn.textContent = name;
        btn.style.setProperty('--keycolor', 'hsl(' + hue(i) + ', 90%, 62%)');
        n.className = 'notename';
        attachKey(btn, i);
        pinKeys[i] = btn;
        noteLabels[i] = n;
        if (side === 'left') slot.append(n, btn); else slot.append(btn, n);
      } else {
        const d = document.createElement('span');
        d.className = 'dead';
        d.textContent = name;
        slot.appendChild(d);
      }
      board.appendChild(slot);
    });
  });
  setNoteNames();
}

// Every status message from the ESP32: which pins are being touched?
function updateMusic(s) {
  if (!info || !info.touchPins || !pinKeys.length) return;
  s.touched.forEach((a, i) => { espOn[i] = a === 1; });
  // Did the ESP32 just restart? Then its counters start at 0 again. Do not play anything, just sync up again.
  if (prevTaps && s.taps.some((n, i) => n < prevTaps[i])) prevTaps = null;
  if (prevTaps) {
    s.taps.forEach((n, i) => {
      if (n === prevTaps[i]) return;
      if (i < 2) {
        // Drum: work out when you really touched the pin (the ESP32 tells us how many ms ago,
        // plus half the round trip time of the message and ~25 ms for smoothing the sensor)
        const moment = s.received - s.rtt / 2 - s.ago[i] - 25;
        if (audioReady() && s.ago[i] < 1000) drumTap(i, audioTimeOf(moment));   // do not play old taps anymore
      } else if (!espOn[i] && !voices[i]) {                // short tap between two measurements
        tapUntil[i] = Date.now() + 250;
        setTimeout(() => updateVoice(i), 260);
      }
    });
  }
  prevTaps = s.taps.slice();
  // BOOT button: the ESP32 counts every press, so we do not miss even a very short press
  if (prevButtonCount !== null && s.buttonCount > prevButtonCount && tab === 'music') {
    if (audioReady()) beatToggle();
    else $('b-status').textContent = 'First tap the button to turn on the sound';
  }
  prevButtonCount = s.buttonCount;
  for (let i = 0; i < info.touchPins.length; i++) {
    updateVoice(i);
    pinKeys[i].style.outlineColor = 'hsla(' + hue(i) + ', 90%, 62%, ' + Math.min(1, s.touch[i] / 40) + ')';
  }
}

// ---------- Connecting buttons and sliders ----------
function fillSelect(id, list) {
  list.forEach(name => { const o = document.createElement('option'); o.textContent = name; $(id).appendChild(o); });
}
fillSelect('m-set', Object.keys(SETS));
fillSelect('z-preset', Object.keys(SOUNDS));
fillSelect('z-scale', Object.keys(SCALES));
fillSelect('c-key', KEYS.map(([name]) => name));
$('c-key').value = 'A minor';
$('m-set').onchange = () => loadSet($('m-set').value);
$('z-preset').onchange = () => { sound = $('z-preset').value; applySettings(); };
$('z-scale').onchange = () => { scale = $('z-scale').value; setNoteNames(); };
$('c-prog').onchange = () => { progression = $('c-prog').value; };   // applies from the next bar
$('c-key').onchange = () => {
  musicKey = KEYS.find(([name]) => name === $('c-key').value)[1];
  setNoteNames();
};
$('z-sound').onclick = startAudioButton;
$('b-start').onclick = () => { startAudioButton(); beatToggle(); };
function setTempoSwing(newBpm, newSwing) {                 // the next hit stays in its place
  const nextT = stepTime(nextN);
  bpm = newBpm; swing = newSwing;
  t0 = nextT - stepOffset(nextN);
  applySettings();
  if (ac && buildStartN >= 0) planBuildSound();            // the drop moves along with the new tempo
}
$('b-tempo').addEventListener('input', e => { setTempoSwing(+e.target.value, swing); $('b-bpm').textContent = bpm; });
$('b-swing').addEventListener('input', e => { setTempoSwing(bpm, +e.target.value); $('b-swingv').textContent = swing + '%'; });
$('m-volume').addEventListener('input', e => { settings.master = +e.target.value; $('m-volumev').textContent = settings.master; applySettings(); });
$('b-clear').onclick = () => { kit.tracks.forEach((_, s) => { for (let i = 0; i < 16; i++) setSymbol(editPattern, s, i, '.'); }); drawGrid(); };
$('b-copy').onclick = () => { clipboard = patterns[editPattern].slice(); $('b-paste').disabled = false; };
$('b-paste').onclick = () => { if (clipboard) { clipboard.forEach((r, s) => { for (let i = 0; i < 16; i++) setSymbol(editPattern, s, i, r[i]); }); drawGrid(); } };
$('r-toggle').onclick = () => { randomOn = !randomOn; drawAll(); };
$('r-wild').addEventListener('input', e => { wildness = +e.target.value; $('r-wildv').textContent = wildness + '%'; });
$('r-dice').onclick = () => {                              // new beat in the pattern you are editing
  generatePattern().forEach((r, s) => { for (let i = 0; i < 16; i++) setSymbol(editPattern, s, i, r[i]); });
  drawGrid();
};
$('r-fill').onclick = () => {                              // fill in this bar if it is still possible, otherwise the next one
  if (!beatOn) return;
  if (nextN % 16 > 0 && nextN % 16 <= 12) fillThisBar = true;
  else fillNext = true;
};
$('r-fills').onchange = () => { fillsOn = $('r-fills').checked; };
$('d-filter').addEventListener('input', e => setDjFilter(+e.target.value));
$('d-center').onclick = () => { $('d-filter').value = 0; setDjFilter(0); };
$('d-build').onclick = () => { startAudioButton(); if (!beatOn) beatStart(); startBuild(); };
const stutterBtn = $('d-stutter');
stutterBtn.addEventListener('pointerdown', e => {
  e.preventDefault();
  if (!beatOn) return;
  stutterStep = Math.max(0, currentStep);
  stutterOn = true;
  stutterBtn.classList.add('on');
});
['pointerup', 'pointerleave', 'pointercancel'].forEach(type => stutterBtn.addEventListener(type, () => {
  stutterOn = false;
  stutterBtn.classList.remove('on');
}));
['float', 'reverb', 'echo', 'bright'].forEach(name => {
  $('z-' + name).addEventListener('input', e => { settings[name] = +e.target.value; applySettings(); });
});
$('z-silence').onclick = () => {
  beatStop();
  screenOn = screenOn.map(() => false);
  Object.keys(voices).forEach(i => noteOff(+i));
};
$('z-calib').onclick = () => {
  const btn = $('z-calib');
  btn.textContent = 'Working...';
  askEsp('/touch/calibrate', 5000)
    .then(() => { btn.textContent = 'Calibrated'; })
    .catch(() => { btn.textContent = 'Failed, try again'; });
};
// ---------- Saving beats in the flash memory of the ESP32 ----------
async function fillSlots() {
  const sel = $('sv-slot'), old = sel.value || '1';
  let list = null;
  try { list = await askEsp('/beat/list', 3000, 'json'); } catch (e) {}
  sel.innerHTML = '';
  for (let n = 1; n <= 4; n++) {
    const o = document.createElement('option');
    o.value = n;
    o.textContent = 'Slot ' + n + (list && list[set.kit] ? (list[set.kit][n - 1] ? ' (saved)' : ' (empty)') : '');
    sel.appendChild(o);
  }
  sel.value = old;
}
$('sv-save').onclick = async () => {
  const slot = $('sv-slot').value, msg = $('sv-info');
  const beat = {version: 1, set: setName, patterns, bpm, swing, progression, key: $('c-key').value};
  msg.textContent = 'Saving...';
  try {
    await askEsp('/beat/save?kit=' + set.kit + '&slot=' + slot, 6000, 'text', {method: 'POST', body: JSON.stringify(beat)});
    msg.textContent = 'Saved to slot ' + slot + ' in the flash memory of the ESP32.';
    fillSlots();
  } catch (e) { msg.textContent = 'Saving did not work. Please try again.'; }
};
// Find the chord progression of a saved beat in the current set. Older (Dutch) beat files use other
// names, like 'Nacht: Am9 Dm9 Fmaj9 Em9'. Then we look for the same chord list after the ': '.
function findProgression(name) {
  if (typeof name !== 'string') return null;
  if (Object.prototype.hasOwnProperty.call(set.progressions, name)) return name;
  const chords = name.split(': ')[1];
  return Object.keys(set.progressions).find(p => chords && p.split(': ')[1] === chords) || null;
}
$('sv-load').onclick = async () => {
  const slot = $('sv-slot').value, msg = $('sv-info');
  let b;
  try { b = await askEsp('/beat/load?kit=' + set.kit + '&slot=' + slot, 6000, 'json'); }
  catch (e) { msg.textContent = 'Slot ' + slot + ' is still empty (or the connection is hiccuping).'; return; }
  // Older (Dutch) beat files use 'patronen', 'progressie' and 'toon' instead of 'patterns', 'progression' and 'key'
  const pats = b ? (b.patterns || b.patronen) : null;
  // First check: is everything correct? A broken file must not make the music get stuck.
  const n = kit.tracks.length;
  const ok = Array.isArray(pats) && pats.length === 4 && pats.every(p =>
    Array.isArray(p) && p.length === n && p.every(r => typeof r === 'string' && /^[.xo234]{16}$/.test(r)));
  if (!ok) { msg.textContent = 'This beat is damaged or belongs to a different version.'; return; }
  patterns = pats.map(p => p.slice());
  setTempoSwing(Math.min(135, Math.max(90, +b.bpm || set.bpm)), Math.min(60, Math.max(0, +b.swing || 0)));
  $('b-tempo').value = bpm; $('b-bpm').textContent = bpm;
  $('b-swing').value = swing; $('b-swingv').textContent = swing + '%';
  const prog = findProgression(b.progression !== undefined ? b.progression : b.progressie);
  if (prog) { progression = prog; $('c-prog').value = progression; }
  const keyName = b.key !== undefined ? b.key : b.toon;    // old files say for example 'A mineur'
  const key = KEYS.find(([name]) => typeof keyName === 'string' && name === keyName.replace('mineur', 'minor'));
  if (key) { musicKey = key[1]; $('c-key').value = key[0]; setNoteNames(); }
  drawAll();
  msg.textContent = 'Beat from slot ' + slot + ' loaded. It plays from the next bar.';
};

loadSet(setName);
requestAnimationFrame(animate);

// ---------------- Reaction game ----------------
$('gm-start').onclick = () => askEsp('/game/start').catch(() => {});
function updateGame(s) {
  const texts = [
    ['Ready?', 'Press Start and watch the LED on your board. Does it turn green? Then press the BOOT button as fast as you can.', ''],
    ['Wait...', 'Do not press yet!', 'red'],
    ['NOW!', 'Press BOOT!', 'green'],
    [s.reaction + ' ms', s.newRecord ? 'New record!' : 'Can you do it even faster?', ''],
    ['Too early!', 'You pressed before the LED turned green, or within 0.1 seconds. No human is that fast: that is guessing.', 'red'],
    ['Too late', 'You did not press for 3 seconds.', '']
  ][s.game] || ['', '', ''];
  $('gm-text').textContent = texts[0];
  $('gm-sub').textContent = texts[1];
  $('gm-box').className = 'gamebox ' + texts[2];
  const busy = s.game === 1 || s.game === 2;
  $('gm-start').disabled = busy;
  $('gm-start').textContent = busy ? 'Busy...' : (s.game === 0 ? 'Start' : 'One more time');
  $('gm-record').textContent = s.record ? s.record + ' ms' : 'none yet';
}

// ---------------- Fractal ----------------
const START = {x: -0.6, y: 0, s: 3.2};
let fx = START.x, fy = START.y, fs = START.s, fractalLoaded = false, fractalBusy = false;
const PALETTE = new Uint8ClampedArray(256 * 3);        // color 0 stays black
for (let i = 1; i < 256; i++) {
  [0, 0.6, 1.0].forEach((d, k) => { PALETTE[i * 3 + k] = 255 * (0.5 + 0.5 * Math.cos(3.0 + i * 0.15 + d)); });
}
function iterations(s) {
  return Math.max(40, Math.min(255, Math.round(80 + 30 * Math.log2(START.s / s))));
}
// Calculates the picture around (nx, ny) with width ns. Only when that worked does it become the new spot.
async function drawFractal(cores, nx = fx, ny = fy, ns = fs) {
  if (!info || fractalBusy) return null;
  fractalBusy = true;
  const c = cores || ($('f-two').checked ? 2 : 1), W = info.fractalW, H = info.fractalH, it = iterations(ns);
  $('f-info').textContent = 'The ESP32 is calculating...';
  try {
    const resp = await askEsp('/fractal?x=' + nx + '&y=' + ny + '&s=' + ns + '&i=' + it + '&c=' + c, 20000, 'bin');
    const ms = parseInt(resp.header('X-Time') || '0', 10), steps = parseInt(resp.header('X-Iter') || '0', 10);
    const data = new Uint8Array(resp.data);
    if (data.length < W * H) throw new Error('too short');
    const cv = $('f-canvas');
    cv.width = W; cv.height = H;
    const ctx = cv.getContext('2d'), img = ctx.createImageData(W, H);
    for (let i = 0; i < W * H; i++) {
      const k = data[i] * 3;
      img.data[i * 4] = PALETTE[k];
      img.data[i * 4 + 1] = PALETTE[k + 1];
      img.data[i * 4 + 2] = PALETTE[k + 2];
      img.data[i * 4 + 3] = 255;
    }
    ctx.putImageData(img, 0, 0);
    fx = nx; fy = ny; fs = ns;
    fractalLoaded = true;
    $('f-info').textContent = 'Calculated in ' + ms + ' ms on ' + c + (c > 1 ? ' cores' : ' core') + ' \u00b7 ' +
      W + 'x' + H + ' pixels \u00b7 max ' + it + ' iterations \u00b7 zoom ' + Math.round(START.s / fs) + 'x' +
      (ms > 0 && steps > 0 ? ' \u00b7 ' + (steps / ms / 1000).toFixed(1) + ' million calculation steps per second' : '');
    return ms;
  } catch (e) {
    $('f-info').textContent = 'That went wrong. Tap again.';
    return null;
  } finally { fractalBusy = false; }
}
$('f-canvas').onclick = e => {
  if (!info || fractalBusy) return;
  const r = e.target.getBoundingClientRect(), step = fs / info.fractalW;
  const nx = fx + ((e.clientX - r.left) * info.fractalW / r.width - info.fractalW / 2) * step;
  const ny = fy + ((e.clientY - r.top) * info.fractalH / r.height - info.fractalH / 2) * step;
  const canZoom = fs / 2 >= 0.0005;                      // any smaller and the chip cannot calculate accurately
  drawFractal(0, nx, ny, canZoom ? fs / 2 : fs).then(ms => {
    if (ms !== null && !canZoom) $('f-info').textContent += ' \u00b7 cannot go deeper: the numbers get too small for the chip';
  });
};
$('f-out').onclick = () => drawFractal(0, fx, fy, Math.min(START.s, fs * 2));
$('f-reset').onclick = () => drawFractal(0, START.x, START.y, START.s);
$('f-compare').onclick = async () => {
  const one = await drawFractal(1);
  const two = await drawFractal(2);
  if (one && two) {
    $('f-info').textContent = '1 core: ' + one + ' ms \u00b7 2 cores: ' + two + ' ms \u00b7 ' + (one / two).toFixed(2) + 'x as fast';
  }
};

// ---------------- WiFi scan ----------------
$('w-scan').onclick = async () => {
  const btn = $('w-scan'), list = $('w-list');
  btn.disabled = true; btn.textContent = 'Searching...'; list.innerHTML = '';
  let done = false;
  for (let attempt = 0; attempt < 25 && !done; attempt++) {
    try {
      const d = await askEsp('/scan', 4000, 'json');
      if (!d.busy) { showNetworks(d.networks); done = true; }
    } catch (e) { /* the connection hiccups during the scan: just try again */ }
    if (!done) await new Promise(r => setTimeout(r, 700));
  }
  if (!done) list.innerHTML = '<p class="explain">No result. Please try again.</p>';
  btn.disabled = false; btn.textContent = 'Search again';
};
function showNetworks(networks) {
  const list = $('w-list');
  networks.sort((a, b) => b.rssi - a.rssi);
  if (!networks.length) { list.innerHTML = '<p class="explain">No networks found.</p>'; return; }
  const head = document.createElement('p');
  head.className = 'explain';
  head.textContent = networks.length + ' networks found, strongest at the top';
  list.appendChild(head);
  networks.forEach(n => {
    list.appendChild(signalRow(n.name || '(hidden network)',
      'channel ' + n.channel + ' \u00b7 ' + n.rssi + ' dBm' + (n.open ? ' \u00b7 open, no password' : ''), n.rssi));
  });
}
// A line with a name, some explanation and 4 bars for the signal strength
function signalRow(nameText, extraText, rssi) {
  const bars = rssi > -55 ? 4 : rssi > -67 ? 3 : rssi > -78 ? 2 : 1;
  const row = document.createElement('div'), left = document.createElement('div');
  const name = document.createElement('div'), extra = document.createElement('div'), strength = document.createElement('div');
  row.className = 'network'; name.className = 'name'; extra.className = 'netinfo'; strength.className = 'strength';
  name.textContent = nameText;
  extra.textContent = extraText;
  for (let b = 1; b <= 4; b++) {
    const s = document.createElement('span');
    s.style.height = (b * 25) + '%';
    if (b <= bars) s.className = 'full';
    strength.appendChild(s);
  }
  left.append(name, extra);
  row.append(left, strength);
  return row;
}

// ---------------- Bluetooth ----------------
$('bt-scan').onclick = async () => {
  const btn = $('bt-scan'), list = $('bt-list');
  btn.disabled = true; btn.textContent = 'Listening...'; list.innerHTML = '';
  let done = false, error = '';
  for (let attempt = 0; attempt < 30 && !done && !error; attempt++) {
    try {
      const d = await askEsp('/ble', 4000, 'json');
      if (d.error) error = d.error;
      else if (!d.busy) { showBle(d.devices); done = true; }
    } catch (e) { /* no connection for a moment: try again */ }
    if (!done && !error) await new Promise(r => setTimeout(r, 700));
  }
  if (!done) {
    const p = document.createElement('p');
    p.className = 'explain';
    p.textContent = error || 'No result. Please try again.';
    list.appendChild(p);
  }
  btn.disabled = false; btn.textContent = 'Search again';
};
function showBle(devices) {
  const list = $('bt-list');
  devices.sort((a, b) => b.rssi - a.rssi);
  const head = document.createElement('p');
  head.className = 'explain';
  head.textContent = devices.length ? devices.length + ' devices found, closest at the top' : 'No devices found.';
  list.appendChild(head);
  devices.forEach(a => {
    const distance = a.rssi > -55 ? 'very close' : a.rssi > -70 ? 'close' : a.rssi > -85 ? 'further away' : 'far away';
    const extra = [a.maker, a.rssi + ' dBm', distance].filter(Boolean).join(' \u00b7 ');
    list.appendChild(signalRow(a.name || '(no name)', extra, a.rssi));
  });
}

// ---------------- Benchmark ----------------
const BENCH_GROUPS = {
  compute: ['Computing', 'The chip calculates with whole numbers (int) lightning fast. For decimal numbers (float) it has its own math helper, the FPU. For extra precise decimal numbers (double) it does not have one: then it does the work step by step with ordinary instructions, and that is much slower.'],
  cores: ['Two cores', 'The same calculation, but now split over both cores at the same time.'],
  simd: ['Vector instructions', 'A vector calculation: multiply 1024 pairs of numbers and add everything up. The ESP32-S3 has special vector instructions that fetch 4 decimal numbers (128 bits) from memory in one go, and then keep track of 4 sums side by side. Useful for sound, images and small AI models.'],
  memory: ['Memory', 'The internal RAM sits inside the chip itself and is the fastest. The PSRAM (8 MB) is a separate memory chip: bigger, but slower. The flash memory (16 MB) stores your program, even without power.'],
  hardware: ['Special hardware', 'For encryption (AES) and for making a fingerprint of data (SHA-256) the chip has its own circuits. It uses them for example for WiFi security. It gets true random numbers from noise of the radio: it has to wait for that each time, so that runs in kB/s.']
};
const fmtNumber = v => v >= 100 ? Math.round(v).toString() : v >= 10 ? v.toFixed(1) : v.toFixed(2);
let benchBusy = false;
$('bm-start').onclick = async () => {
  if (benchBusy) return;
  benchBusy = true;
  const btn = $('bm-start'), list = $('bm-list'), concl = $('bm-conclusion');
  btn.disabled = true; list.innerHTML = ''; concl.style.display = 'none';
  const results = {}, groups = {};
  let error = '';
  for (let t = 0; t < 40; t++) {
    btn.textContent = 'Test ' + (t + 1) + ' running...';
    let r;
    try { r = await askEsp('/benchmark?t=' + t, 15000, 'json'); }
    catch (e) {
      if (e.message !== 'HTTP 404') error = 'Test ' + (t + 1) + ' did not work (no connection?). Please try again.';
      break;                                               // 404 = there are no more tests
    }
    results[r.id] = r.value;
    if (!groups[r.group]) {                                // new block for this group
      const [title, explanation] = BENCH_GROUPS[r.group] || [r.group, ''];
      const box = document.createElement('div'), h = document.createElement('h3'), p = document.createElement('p');
      box.className = 'card bgroup'; p.className = 'explain'; p.style.margin = '0';
      h.textContent = title; p.textContent = explanation;
      box.append(h, p);
      list.appendChild(box);
      groups[r.group] = {box, rows: []};
    }
    const g = groups[r.group], row = document.createElement('div'), head = document.createElement('div');
    const name = document.createElement('span'), value = document.createElement('span');
    const bar = document.createElement('div'), fill = document.createElement('div');
    row.className = 'brow'; head.className = 'row'; bar.className = 'bbar';
    name.textContent = r.name; value.textContent = fmtNumber(r.value) + ' ' + r.unit;
    head.append(name, value); bar.appendChild(fill); row.append(head, bar);
    g.box.appendChild(row);
    g.rows.push({fill, value: r.value, unit: r.unit});
    // the best one in the group gets a full bar (only compared with the same unit)
    g.rows.forEach(x => {
      const max = Math.max(...g.rows.filter(y => y.unit === x.unit).map(y => y.value));
      x.fill.style.width = (max > 0 ? x.value / max * 100 : 0) + '%';
    });
    if (r.last) break;
  }
  // What stands out? A few comparisons
  const times = (a, b) => results[a] > 0 && results[b] > 0 ? (results[a] / results[b]).toFixed(1).replace('.0', '') + ' times' : null;
  const sentences = [
    [times('float', 'double'), 'Decimal numbers: with the FPU (float) it is # as fast as without it (double).'],
    [times('cores2', 'cores1'), 'With two cores at the same time it is # as fast as with one core.'],
    [times('simd', 'plain'), 'With vector instructions it is # as fast as with plain C code.'],
    [times('sram', 'psram'), 'The internal RAM is # as fast as the PSRAM.']
  ].filter(([w]) => w).map(([w, z]) => z.replace('#', w));
  if (error) sentences.unshift(error);
  if (sentences.length) {
    concl.innerHTML = '<b>What stands out?</b>';
    sentences.forEach(z => { const d = document.createElement('div'); d.textContent = z; d.style.marginTop = '6px'; concl.appendChild(d); });
    concl.style.display = 'block';
  }
  btn.disabled = false; btn.textContent = 'Test again';
  benchBusy = false;
};

// ---------------- Info ----------------
async function loadInfo() {
  try {
    info = await askEsp('/info', 3000, 'json');
  } catch (e) { setTimeout(loadInfo, 2000); return; }
  $('wifi-name').textContent = info.wifi;
  const rows = [
    ['Chip', info.chip + ' (revision ' + info.revision + ')'],
    ['Processor', info.cores + ' cores \u00b7 ' + info.mhz + ' MHz'],
    ['Flash memory', bytes(info.flash)],
    ['Working memory (RAM)', bytes(info.ram)],
    ['PSRAM (extra memory)', info.psramTotal ? bytes(info.psramTotal) : 'not found'],
    ['WiFi network', info.wifi + (info.channel ? ' (channel ' + info.channel + ', the quietest one at startup)' : '')],
    ['MAC address', info.mac],
    ['ESP-IDF version', info.sdk],
    ['Fractal resolution', info.fractalW + ' x ' + info.fractalH],
    ['Storage for beats (FAT)', info.storageTotal ? bytes(info.storageFree) + ' free of ' + bytes(info.storageTotal) : 'not available'],
    ['Last restart', info.resetReason || '-']
  ];
  const table = $('i-table');
  table.innerHTML = '';
  rows.forEach(([a, b]) => {
    const tr = document.createElement('tr'), th = document.createElement('th'), td = document.createElement('td');
    th.textContent = a; td.textContent = b;
    tr.append(th, td);
    table.appendChild(tr);
  });
  buildBoard();
  if (tab === 'fractal' && !fractalLoaded) drawFractal();
}
let restartSure = false;
$('i-restart').onclick = () => {
  const btn = $('i-restart');
  if (!restartSure) {
    restartSure = true;
    btn.textContent = 'Are you sure? Tap again';
    setTimeout(() => { restartSure = false; if (!btn.disabled) btn.textContent = 'Restart ESP32'; }, 3000);
    return;
  }
  btn.textContent = 'Restarting...';
  btn.disabled = true;
  askEsp('/restart').catch(() => {});
  setTimeout(() => location.reload(), 6000);
};

// Task list: refresh every 2 seconds while the Info tab is open
async function loadTasks() {
  if (tab !== 'info') return;
  let d;
  try { d = await askEsp('/tasks', 3000, 'json'); } catch (e) { return; }
  const table = $('i-tasks');
  table.innerHTML = '';
  const row = (values, head) => {
    const tr = document.createElement('tr');
    values.forEach((w, i) => { const c = document.createElement(head || i === 0 ? 'th' : 'td'); c.textContent = w; tr.appendChild(c); });
    table.appendChild(tr);
  };
  row(['Task', 'Core', 'Prio', 'Busy', 'Stack free'], true);
  d.tasks.sort((a, b) => b.cpu - a.cpu || a.name.localeCompare(b.name)).forEach(t => {
    row([t.name, t.core < 0 ? 'both' : t.core, t.prio, t.cpu < 0 ? '?' : t.cpu.toFixed(1) + '%', t.stack + ' B']);
  });
}
setInterval(loadTasks, 2000);

loadInfo();
liveStart();
statusLoop();
</script>
</body>
</html>
)rawliteral";
