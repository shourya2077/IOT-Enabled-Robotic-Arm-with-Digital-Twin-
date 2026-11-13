#pragma once
/** WebPage.h - the phone dashboard (HTML + CSS + JS) stored in flash. Pure data, no logic in C++. */
#include <Arduino.h>

static const char PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="theme-color" content="#0b1220"><title>DigiArm</title>
<style>
:root{--bg:#0b1220;--card:#131c2e;--line:#22304a;--txt:#e6edf7;--mut:#8fa1bd;--acc:#22d3ee;--ok:#22c55e;--bad:#ef4444;--warn:#f59e0b}
*{box-sizing:border-box}[hidden]{display:none!important}
body{margin:0 auto;max-width:600px;padding:12px;background:var(--bg);color:var(--txt);font:15px system-ui,sans-serif}
h1{font-size:20px;margin:0;flex:1}.top{display:flex;align-items:center;gap:8px;margin-bottom:10px}
.dot{width:10px;height:10px;border-radius:50%;background:var(--bad)}.dot.ok{background:var(--ok)}
.badge{padding:3px 9px;border-radius:99px;background:var(--line);font-size:12px;letter-spacing:.5px}
.badge.moving,.badge.homing{background:#1e40af}.badge.estop{background:var(--bad)}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:10px 12px;margin-bottom:10px}
.row{display:flex;align-items:center;justify-content:space-between;gap:8px}.mut{color:var(--mut);font-size:12px}
.row3{display:grid;grid-template-columns:1.2fr 1fr 1fr;gap:8px;margin-bottom:10px}
button{background:var(--line);color:var(--txt);border:0;border-radius:10px;padding:10px 12px;font-size:14px}
button:active{filter:brightness(1.3)}button.big{padding:16px 6px;font-weight:700}
button.red{background:var(--bad)}button.acc{background:var(--acc);color:#04222a;font-weight:700}
button.nb{padding:6px 10px;min-width:44px}
.seg{display:grid;grid-template-columns:1fr 1fr;gap:6px;margin-bottom:10px}.seg button.on{background:var(--acc);color:#04222a;font-weight:700}
input[type=range]{width:100%;height:36px;accent-color:var(--acc)}input[type=range]:disabled{opacity:.35}
input[type=number],input[type=text],input[type=password],select{width:100%;background:var(--bg);color:var(--txt);border:1px solid var(--line);border-radius:8px;padding:7px;font-size:14px}
input[type=checkbox]{width:20px;height:20px;accent-color:var(--acc)}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px}
label.f{display:flex;flex-direction:column;gap:3px}label.f>span{color:var(--mut);font-size:12px}
.wp{display:flex;align-items:center;gap:6px;padding:6px 0;border-top:1px solid var(--line);font-size:12px}
.wp.cur{background:#0e3a4a}.wp span{flex:1;font-family:ui-monospace,monospace}
.calc{margin-top:8px;font-size:12px;color:var(--mut);font-family:ui-monospace,monospace}.calc.warn{color:var(--warn)}
#toast{position:fixed;left:50%;bottom:16px;transform:translateX(-50%);background:var(--warn);color:#111;padding:8px 14px;border-radius:99px;display:none;font-weight:600;max-width:90%;text-align:center}
</style></head><body>
<div class="top"><h1>DigiArm</h1><span id="link" class="dot"></span><span id="st" class="badge">connecting</span></div>
<div class="row3">
 <button class="big red" onclick="act('/api/estop')">E-STOP</button>
 <button class="big" onclick="act('/api/enable?on=1')">Reset / Enable</button>
 <button class="big" onclick="act('/api/home')">Home</button>
</div>
<div class="seg"><button id="tCtl" class="on" onclick="tab('ctl')">Control</button><button id="tSet" onclick="tab('set')">Settings</button></div>

<div id="pCtl">
 <div class="card"><div class="row"><span>Speed</span><b id="spv">50%</b></div>
  <input type="range" id="spd" min="5" max="100" value="50"></div>
 <div class="seg"><button id="mManual" onclick="act('/api/learn?on=0')">Manual</button><button id="mLearn" onclick="act('/api/learn?on=1')">Learn</button></div>
 <div id="joints"></div>
 <div id="learn" class="card" hidden>
  <div class="row"><b>Teach mode</b><span class="mut"><span id="wpn">0</span> waypoints</span></div>
  <p class="mut" style="margin:6px 0">Move the sliders to a pose, then save it. Poses replay in order.</p>
  <div class="row"><span class="mut">Pause after pose (ms)</span><input type="number" id="dwell" value="300" min="0" max="10000" step="100" style="width:90px"></div>
  <div class="row" style="margin-top:8px"><button class="acc" style="flex:1" onclick="act('/api/save?dwell='+$('dwell').value)">+ Save pose</button>
  <button onclick="if(confirm('Delete ALL waypoints?'))act('/api/clear')">Clear all</button></div>
  <div id="wps"></div>
 </div>
 <div class="card"><div class="row"><b>Playback</b>
  <label class="mut"><input type="checkbox" id="loop"> loop</label></div>
  <div class="row" style="margin-top:8px"><button class="acc" style="flex:1" onclick="act('/api/play?loop='+($('loop').checked?1:0))">&#9654; Play</button>
  <button style="flex:1" onclick="act('/api/stop')">&#9632; Stop</button></div></div>
 <div class="card"><div class="row"><span>MQTT: <b id="mq">-</b></span>
  <label class="mut"><input type="checkbox" id="mqctl" checked> allow remote control</label></div>
  <div class="row" style="margin-top:8px"><button onclick="if(confirm('Declare the CURRENT pose as 0 / home?'))act('/api/zero')">Set home here</button>
  <button onclick="if(confirm('Motors OFF: the arm may fall. Continue?'))act('/api/enable?on=0')">Motors OFF</button></div></div>
</div>

<div id="pSet" hidden>
 <div class="card"><div class="row"><b>Motor &amp; joint settings</b></div>
  <p class="mut" style="margin:6px 0 0">Applies instantly and is stored in flash. Microstep must match the MS jumpers under each driver. Changing gear ratio / microstep changes how angles are computed from steps: re-teach saved poses afterwards.</p></div>
 <div id="sj"></div>
 <button class="acc" style="width:100%;margin-bottom:10px" onclick="saveJoints()">Save &amp; apply joint settings</button>
 <div class="card"><div class="row"><b>Wi-Fi client &amp; MQTT</b><span class="mut" id="netinfo"></span></div>
  <div class="grid2">
   <label class="f"><span>Join router / hotspot</span><input type="checkbox" id="nSta"></label><span></span>
   <label class="f"><span>Wi-Fi name (SSID)</span><input type="text" id="nSsid" maxlength="32"></label>
   <label class="f"><span>Wi-Fi password</span><input type="password" id="nPass" maxlength="64"></label>
   <label class="f"><span>MQTT broker host / IP</span><input type="text" id="nHost" maxlength="63"></label>
   <label class="f"><span>MQTT port (TCP)</span><input type="number" id="nPort" min="1" max="65535"></label>
   <label class="f"><span>MQTT user</span><input type="text" id="nUser" maxlength="32"></label>
   <label class="f"><span>MQTT password</span><input type="password" id="nMpass" maxlength="64"></label>
   <label class="f"><span>Telemetry topic</span><input type="text" id="nTTel" maxlength="47"></label>
   <label class="f"><span>Command topic</span><input type="text" id="nTCmd" maxlength="47"></label>
   <label class="f"><span>Status topic</span><input type="text" id="nTSt" maxlength="47"></label>
  </div>
  <button class="acc" style="width:100%;margin-top:10px" onclick="saveNet()">Save &amp; reconnect</button></div>
 <div class="card"><div class="row"><b>Device</b></div>
  <div class="row" style="margin-top:8px"><button onclick="if(confirm('Restore factory defaults for ALL settings?'))post('/api/reset',{}).then(r=>toast(r&&r.ok?'Defaults restored':'Failed'))">Factory reset settings</button>
  <button onclick="if(confirm('Reboot the ESP32? (the arm must be back in HOME pose afterwards)'))act('/api/reboot')">Reboot</button></div>
  <p class="mut" style="margin:8px 0 0">Saves the current Speed slider value as the power-on default when you press &ldquo;Save &amp; apply joint settings&rdquo;. Pins and the access-point password are in Config.h.</p></div>
</div>
<div id="toast"></div>
<script>
const $=id=>document.getElementById(id);
let CFG=null,drag={},last={},tmr={},wpVer=-1,cfgVer=-1,dragSpd=false;
const api=p=>fetch(p).then(r=>r.json()).catch(()=>null);
const post=(p,b)=>fetch(p,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)}).then(r=>r.json()).catch(()=>null);
const esc=s=>String(s).replace(/&/g,'&amp;').replace(/"/g,'&quot;').replace(/</g,'&lt;');
function toast(m){const t=$('toast');t.textContent=m;t.style.display='block';clearTimeout(t._h);t._h=setTimeout(()=>t.style.display='none',2600)}
function act(p){api(p).then(j=>{if(j&&!j.ok&&j.err)toast(j.err)})}
function tab(n){$('pCtl').hidden=n!=='ctl';$('pSet').hidden=n!=='set';$('tCtl').classList.toggle('on',n==='ctl');$('tSet').classList.toggle('on',n==='set')}
function send(i,v){last[i]=Date.now();api('/api/move?j='+i+'&v='+v).then(j=>{if(j&&!j.ok&&j.err)toast(j.err)})}
function throttled(i,v){const dt=Date.now()-(last[i]||0);clearTimeout(tmr[i]);if(dt>=80)send(i,v);else tmr[i]=setTimeout(()=>send(i,v),80-dt)}
function nudge(i,d){const s=$('s'+i);s.value=Math.max(+s.min,Math.min(+s.max,+s.value+d));$('t'+i).textContent=(+s.value).toFixed(1);send(i,s.value)}

/* ---------- control tab ---------- */
function buildControl(){const box=$('joints');box.innerHTML='';
 CFG.joints.forEach((p,i)=>{const d=document.createElement('div');d.className='card';
  d.innerHTML='<div class="row"><b>'+esc(p.name)+'</b><span class="mut">now <span id="a'+i+'">0.0</span>&deg; &rarr; <span id="t'+i+'">0.0</span>&deg;</span></div>'+
  '<div class="row"><button class="nb" onclick="nudge('+i+',-1)">-1</button><input type="range" id="s'+i+'" min="'+p.min+'" max="'+p.max+'" step="0.5" value="0"><button class="nb" onclick="nudge('+i+',1)">+1</button></div>'+
  '<div class="row mut"><span>'+p.min+'&deg;</span><span>'+p.max+'&deg;</span></div>';
  box.appendChild(d);const s=d.querySelector('input');
  s.addEventListener('input',()=>{drag[i]=true;$('t'+i).textContent=(+s.value).toFixed(1);throttled(i,s.value)});
  s.addEventListener('change',()=>{clearTimeout(tmr[i]);send(i,s.value);setTimeout(()=>drag[i]=false,500)});
 });}
async function loadWps(){const w=await api('/api/wps');if(!w)return;const box=$('wps');box.innerHTML='';
 w.forEach((p,i)=>{const e=document.createElement('div');e.className='wp';
  e.innerHTML='<b>'+(i+1)+'</b><span>'+p.q.map(v=>v.toFixed(0)).join(' / ')+' &middot; '+p.d+'ms</span>'+
  '<button onclick="act(\'/api/wp/go?i='+i+'\')">Go</button><button onclick="act(\'/api/wp/del?i='+i+'\')">&times;</button>';
  box.appendChild(e)})}

/* ---------- settings tab ---------- */
const NUM=(id,v,a)=>'<input type="number" id="'+id+'" value="'+v+'" '+(a||'step="any"')+'>';
const SEL=(id,opts,v)=>'<select id="'+id+'">'+opts.map(o=>'<option'+(o==v?' selected':'')+'>'+o+'</option>').join('')+'</select>';
const FLD=(l,h)=>'<label class="f"><span>'+l+'</span>'+h+'</label>';
function calc(i){const g=+$('g'+i).value,r=+$('r'+i).value,m=+$('m'+i).value,v=+$('vs'+i).value;
 const spd=r*m*g/360,peak=v*spd,el=$('dv'+i);
 el.textContent='steps/deg '+spd.toFixed(2)+'  |  resolution '+(1/spd).toFixed(4)+' deg/step  |  peak '+Math.round(peak)+' steps/s'+(peak>CFG.maxHz?'  (CAPPED at '+CFG.maxHz+')':'');
 el.className='calc'+(peak>CFG.maxHz?' warn':'')}
function buildSettings(){const box=$('sj');box.innerHTML='';
 CFG.joints.forEach((p,i)=>{const d=document.createElement('div');d.className='card';
  d.innerHTML='<div class="row"><b>Joint '+i+'</b><input type="text" id="n'+i+'" value="'+esc(p.name)+'" maxlength="23" style="max-width:220px"></div>'+
  '<div class="grid2">'+
  FLD('Gear ratio (total)',NUM('g'+i,p.gear,'step="0.01" min="0.01"'))+
  FLD('Motor steps/rev',SEL('r'+i,[200,400],p.spr))+
  FLD('Microstep 1/',SEL('m'+i,[1,2,4,8,16,32],p.ms))+
  FLD('Reverse direction','<input type="checkbox" id="v'+i+'"'+(p.inv?' checked':'')+'>')+
  FLD('Min angle (deg)',NUM('lo'+i,p.min))+FLD('Max angle (deg)',NUM('hi'+i,p.max))+
  FLD('Max speed (deg/s)',NUM('vs'+i,p.vmax))+FLD('Max accel (deg/s&sup2;)',NUM('ac'+i,p.amax))+
  '</div><div class="calc" id="dv'+i+'"></div>'+
  '<div class="row" style="margin-top:8px"><span class="mut">Direction / ratio test:</span><span><button onclick="act(\'/api/jog?j='+i+'&d=-5\')">-5&deg;</button> <button onclick="act(\'/api/jog?j='+i+'&d=5\')">+5&deg;</button></span></div>';
  box.appendChild(d);
  ['g','r','m','vs'].forEach(k=>$(k+i).addEventListener('input',()=>calc(i)));calc(i)});
 const n=CFG.net;$('nSta').checked=n.sta;$('nSsid').value=n.ssid;$('nPass').value=n.pass;$('nHost').value=n.host;$('nPort').value=n.port;
 $('nUser').value=n.user;$('nMpass').value=n.mpass;$('nTTel').value=n.tTel;$('nTCmd').value=n.tCmd;$('nTSt').value=n.tSt}
function collectJoints(){return CFG.joints.map((_,i)=>({name:$('n'+i).value,gear:+$('g'+i).value,spr:+$('r'+i).value,ms:+$('m'+i).value,
 inv:$('v'+i).checked,min:+$('lo'+i).value,max:+$('hi'+i).value,vmax:+$('vs'+i).value,amax:+$('ac'+i).value}))}
async function saveJoints(){const r=await post('/api/settings',{joints:collectJoints(),speed:$('spd').value/100});toast(r&&r.ok?'Saved & applied':(r&&r.err)||'Failed')}
async function saveNet(){const r=await post('/api/settings',{net:{sta:$('nSta').checked,ssid:$('nSsid').value,pass:$('nPass').value,host:$('nHost').value,
 port:+$('nPort').value,user:$('nUser').value,mpass:$('nMpass').value,tTel:$('nTTel').value,tCmd:$('nTCmd').value,tSt:$('nTSt').value}});
 toast(r&&r.ok?'Saved - reconnecting':(r&&r.err)||'Failed')}
async function loadCfg(){const c=await api('/api/settings');if(!c)return;CFG=c;buildControl();buildSettings()}

/* ---------- live state ---------- */
function render(s){
 $('link').className='dot ok';$('st').textContent=s.state.toUpperCase()+' \u00b7 '+s.mode.toUpperCase()+(s.en?'':' \u00b7 MOTORS OFF');
 $('st').className='badge '+s.state;
 if(s.cfgver!==cfgVer){cfgVer=s.cfgver;loadCfg()}
 s.q.forEach((v,i)=>{const sl=$('s'+i);if(!sl)return;$('a'+i).textContent=v.toFixed(1);if(!drag[i]){sl.value=s.tgt[i];$('t'+i).textContent=s.tgt[i].toFixed(1)}});
 $('mManual').classList.toggle('on',s.mode!=='learn');$('mLearn').classList.toggle('on',s.mode==='learn');
 $('learn').hidden=s.mode!=='learn';$('wpn').textContent=s.wpn;
 if(s.wpver!==wpVer){wpVer=s.wpver;loadWps()}
 document.querySelectorAll('.wp').forEach((e,i)=>e.classList.toggle('cur',s.mode==='play'&&i===s.idx));
 document.querySelectorAll('#joints input').forEach(e=>e.disabled=(s.mode==='play'||s.state==='estop'||!s.en));
 $('mq').textContent=s.mq;$('netinfo').textContent=s.ip?('Wi-Fi IP '+s.ip):'';
 if(document.activeElement!==$('mqctl'))$('mqctl').checked=s.mqctl;
 if(!dragSpd){$('spd').value=Math.round(s.spd*100);$('spv').textContent=Math.round(s.spd*100)+'%'}
}
async function poll(){const s=await api('/api/state');if(s)render(s);else{$('link').className='dot';$('st').textContent='OFFLINE'}setTimeout(poll,300)}
(function init(){
 const sp=$('spd');sp.addEventListener('input',()=>{dragSpd=true;$('spv').textContent=sp.value+'%'});
 sp.addEventListener('change',()=>{act('/api/speed?v='+(sp.value/100));setTimeout(()=>dragSpd=false,500)});
 $('mqctl').addEventListener('change',e=>act('/api/mqttctl?on='+(e.target.checked?1:0)));
 poll();
})();
</script></body></html>)rawliteral";
