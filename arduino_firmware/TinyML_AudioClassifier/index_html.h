#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>TinyML Audio Classifier</title>
<style>
  :root {
    --bg: #f8fafc;
    --card: #ffffff;
    --text: #0f172a;
    --text-muted: #64748b;
    --primary: #2563eb;
    --primary-hover: #1d4ed8;
    --border: #e2e8f0;
    --table-header: #f1f5f9;
    --table-highlight: #ecfdf5;
    --table-highlight-border: #a7f3d0;
    --success: #16a34a;
    --danger: #dc2626;
  }
  * { box-sizing: border-box; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; margin: 0; padding: 0; }
  body { background: var(--bg); color: var(--text); padding: 2rem 1rem 4rem; display: flex; justify-content: center; }
  .container { width: 100%; max-width: 680px; display: flex; flex-direction: column; gap: 1.5rem; }
  
  .card { background: var(--card); border-radius: 12px; padding: 2rem; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.07), 0 2px 4px -2px rgba(0,0,0,0.05); border: 1px solid var(--border); }
  h1 { font-size: 1.75rem; font-weight: 700; margin-bottom: 0.25rem; color: var(--text); }
  .subtitle { color: var(--text-muted); font-size: 0.95rem; margin-bottom: 1.5rem; }
  
  .drop-zone { border: 2px dashed #cbd5e1; border-radius: 10px; padding: 2rem 1rem; text-align: center; cursor: pointer; transition: all 0.2s ease; background: #f8fafc; margin-bottom: 1rem; }
  .drop-zone:hover, .drop-zone.dragover { border-color: var(--primary); background: #eff6ff; }
  .file-input { display: none; }
  
  .btn-row { display: flex; gap: 0.75rem; }
  .btn { background: var(--primary); color: white; border: none; padding: 0.75rem 1.25rem; border-radius: 8px; font-weight: 600; cursor: pointer; transition: background 0.2s; font-size: 0.95rem; flex: 1; }
  .btn:hover { background: var(--primary-hover); }
  .btn:disabled { background: #94a3b8; cursor: not-allowed; }
  .btn.secondary { background: #e2e8f0; color: #334155; }
  .btn.secondary:hover { background: #cbd5e1; }
  .btn.danger { background: var(--danger); color: white; }
  
  .toggle-group { display: flex; gap: 0.5rem; margin-bottom: 1rem; }
  .toggle-btn { flex: 1; padding: 0.4rem; border: 1px solid var(--border); background: white; color: var(--text); border-radius: 6px; cursor: pointer; font-size: 0.85rem; font-weight: 500; }
  .toggle-btn.active { background: var(--primary); color: white; border-color: var(--primary); }
  
  .canvas-container { width: 100%; background: #0f172a; border-radius: 8px; padding: 0.75rem; margin: 1rem 0; }
  canvas { width: 100%; height: 70px; display: block; }
  
  .spinner { border: 3px solid rgba(0,0,0,0.1); border-left-color: var(--primary); border-radius: 50%; width: 32px; height: 32px; animation: spin 1s linear infinite; margin: 1.5rem auto; display: none; }
  @keyframes spin { to { transform: rotate(360deg); } }
  
  /* Result styling matching reference */
  .result-section { display: none; }
  .result-section.active { display: block; }
  
  .result-header { font-size: 1.5rem; font-weight: 700; margin-bottom: 1.25rem; }
  .result-summary { font-size: 1.15rem; margin-bottom: 0.4rem; }
  .result-summary strong { font-weight: 600; }
  
  .table-title { font-size: 1.15rem; font-weight: 700; margin-top: 1.75rem; margin-bottom: 0.75rem; }
  .timing-table { width: 100%; border-collapse: collapse; margin-bottom: 0.5rem; font-size: 0.95rem; }
  .timing-table th, .timing-table td { padding: 0.65rem 0.85rem; text-align: left; border: 1px solid #e2e8f0; }
  .timing-table th { background: var(--table-header); font-weight: 600; }
  .timing-table td:last-child, .timing-table th:last-child { width: 120px; }
  .timing-table tr.highlight { background: var(--table-highlight); font-weight: 700; border-top: 2px solid var(--table-highlight-border); }
  
  .note-text { font-size: 0.825rem; color: var(--text-muted); line-height: 1.4; margin-bottom: 1.25rem; }
  .link-upload-another { color: #6d28d9; text-decoration: underline; font-weight: 500; cursor: pointer; display: inline-block; font-size: 1rem; margin-top: 0.5rem; }
  
  /* Top 5 bar breakdown */
  .top5-container { margin-top: 1.5rem; padding-top: 1rem; border-top: 1px solid var(--border); }
  .top5-title { font-size: 0.95rem; font-weight: 600; color: var(--text-muted); margin-bottom: 0.75rem; }
  .pred-row { display: flex; align-items: center; margin-bottom: 0.4rem; font-size: 0.85rem; }
  .pred-name { width: 110px; text-transform: capitalize; font-weight: 500; }
  .pred-bar-bg { flex: 1; background: #f1f5f9; height: 8px; border-radius: 4px; margin: 0 0.75rem; overflow: hidden; }
  .pred-bar { height: 100%; background: var(--primary); border-radius: 4px; }
  .pred-val { width: 45px; text-align: right; color: var(--text-muted); font-variant-numeric: tabular-nums; }
  
  .status-footer { display: flex; justify-content: space-between; font-size: 0.8rem; color: var(--text-muted); padding-top: 0.5rem; border-top: 1px solid var(--border); margin-top: 1rem; }
  .status-dot { width: 8px; height: 8px; border-radius: 50%; display: inline-block; margin-right: 0.35rem; }
  .status-dot.connected { background: var(--success); }
  .status-dot.disconnected { background: var(--danger); }
  
  .toast { position: fixed; bottom: 1.5rem; right: 1.5rem; background: #0f172a; color: white; padding: 0.75rem 1.25rem; border-radius: 8px; font-size: 0.9rem; opacity: 0; transform: translateY(20px); transition: all 0.3s; pointer-events: none; z-index: 100; }
  .toast.show { opacity: 1; transform: translateY(0); }
</style>
</head>
<body>
<div class="container">

  <!-- Input & Control Card -->
  <div class="card" id="inputCard">
    <h1>🎵 TinyML Audio Classifier</h1>
    <p class="subtitle">ESP32 On-Device Sound Recognition &amp; Benchmarking</p>
    
    <div class="drop-zone" id="dropZone">
      <p style="font-weight: 600; margin-bottom: 0.25rem;">📁 Tap to Record or Upload Audio</p>
      <p style="font-size: 0.85rem; color: var(--text-muted);">Select a .wav/.mp3/.m4a file or record a voice clip</p>
      <input type="file" id="fileInput" class="file-input" accept="audio/*,.wav,.mp3,.m4a,.ogg,.aac">
    </div>
    <p id="fileName" style="font-size: 0.85rem; color: var(--primary); margin-bottom: 0.75rem; text-align: center; font-weight: 500;"></p>

    <div style="text-align:center; margin: 0.5rem 0 1rem; color: var(--text-muted); font-size: 0.85rem;">— OR RECORD LIVE AUDIO —</div>

    <div class="toggle-group" id="durationToggle">
      <button class="toggle-btn active" data-dur="1">1s Duration</button>
      <button class="toggle-btn" data-dur="2">2s Duration</button>
      <button class="toggle-btn" data-dur="3">3s Duration</button>
    </div>

    <div class="btn-row">
      <button class="btn secondary" id="btnRecord">🎤 Start Recording</button>
      <button class="btn" id="btnClassify" disabled>⚡ Classify Audio</button>
    </div>

    <div class="canvas-container">
      <canvas id="waveform"></canvas>
    </div>

    <div class="status-footer">
      <div><span class="status-dot connected" id="connDot"></span> <span id="connText">ESP32 Connected</span></div>
      <div id="statInfo">Free Heap: -- KB</div>
    </div>
  </div>

  <!-- Results Card -->
  <div class="card result-section" id="resultCard">
    <div class="spinner" id="spinner"></div>
    <div id="resultDetails" style="display:none;">
      <h2 class="result-header">Audio uploaded and processed.</h2>

      <p class="result-summary">Prediction: <strong id="resPredName" style="text-transform: capitalize;">--</strong></p>
      <p class="result-summary" style="margin-bottom: 1.5rem;">Confidence: <strong id="resConfVal">--%</strong></p>

      <h3 class="table-title">Timing breakdown (measured on the ESP32)</h3>
      <table class="timing-table">
        <thead>
          <tr>
            <th>Stage</th>
            <th>Duration</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td>1. File upload (transfer to device)</td>
            <td id="tUpload">-- s</td>
          </tr>
          <tr>
            <td>2. WAV parsing</td>
            <td id="tParse">-- s</td>
          </tr>
          <tr>
            <td>3. Resampling (44.1kHz &rarr; 16kHz)</td>
            <td id="tResample">-- s</td>
          </tr>
          <tr>
            <td>4. Mel spectrogram extraction</td>
            <td id="tMel">-- s</td>
          </tr>
          <tr>
            <td>5. Model inference</td>
            <td id="tInf">-- s</td>
          </tr>
          <tr class="highlight">
            <td>Total, device-side (upload start &rarr; result ready)</td>
            <td id="tDeviceTotal">-- s</td>
          </tr>
        </tbody>
      </table>

      <h3 class="table-title">Timing (measured in the browser)</h3>
      <table class="timing-table">
        <thead>
          <tr>
            <th>Metric</th>
            <th>Duration</th>
          </tr>
        </thead>
        <tbody>
          <tr class="highlight">
            <td>Click "Upload" &rarr; result page rendered</td>
            <td id="tBrowserTotal">-- s</td>
          </tr>
        </tbody>
      </table>
      <p class="note-text">The browser timing includes network transfer both ways plus the device-side total above, so it will always be a little larger than the device-side total.</p>

      <a class="link-upload-another" id="btnUploadAnother">Upload another file</a>

      <div class="top5-container">
        <div class="top5-title">Top 5 Predictions Breakdown</div>
        <div id="top5List"></div>
      </div>
    </div>
  </div>

</div>

<div class="toast" id="toast">Message</div>

<script>
const TARGET_SAMPLE_RATE = 16000;
const emojiMap = {
  dog: '🐕', cat: '🐱', cow: '🐄', frog: '🐸', rooster: '🐓', pig: '🐷', sheep: '🐑', crow: '🦅',
  rain: '🌧️', sea_waves: '🌊', crackling_fire: '🔥', wind: '💨', thunderstorm: '⛈️', pouring_water: '💧',
  clapping: '👏', breathing: '😤', coughing: '🤧', footsteps: '👣', laughing: '😂', snoring: '😴', sneezing: '🤧'
};

let currentAudioPCM = null;
let audioContext = null;
let clientResampleTimeSec = 0.000;

// UI References
const dropZone = document.getElementById('dropZone');
const fileInput = document.getElementById('fileInput');
const fileName = document.getElementById('fileName');
const btnClassify = document.getElementById('btnClassify');
const btnRecord = document.getElementById('btnRecord');
const btnUploadAnother = document.getElementById('btnUploadAnother');
const durationBtns = document.querySelectorAll('.toggle-btn');
const inputCard = document.getElementById('inputCard');
const resultCard = document.getElementById('resultCard');
const spinner = document.getElementById('spinner');
const resultDetails = document.getElementById('resultDetails');
const canvas = document.getElementById('waveform');
const ctx = canvas.getContext('2d');

function showToast(msg) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), 3000);
}

function floatTo16BitPCM(input) {
  const output = new Int16Array(input.length);
  for (let i = 0; i < input.length; i++) {
    const s = Math.max(-1, Math.min(1, input[i]));
    output[i] = s < 0 ? s * 0x8000 : s * 0x7FFF;
  }
  return output;
}

function drawWaveform(pcmData) {
  const w = canvas.width, h = canvas.height;
  ctx.fillStyle = '#0f172a';
  ctx.fillRect(0, 0, w, h);
  if (!pcmData || pcmData.length === 0) return;
  ctx.lineWidth = 2;
  ctx.strokeStyle = '#38bdf8';
  ctx.beginPath();
  const step = Math.ceil(pcmData.length / w);
  for (let i = 0; i < w; i++) {
    let min = 1.0, max = -1.0;
    for (let j = 0; j < step; j++) {
      const idx = i * step + j;
      if (idx < pcmData.length) {
        const val = pcmData[idx] / 32768.0;
        if (val < min) min = val;
        if (val > max) max = val;
      }
    }
    const yMax = (1 - max) * h / 2;
    const yMin = (1 - min) * h / 2;
    if (i === 0) ctx.moveTo(i, yMax);
    else { ctx.lineTo(i, yMax); ctx.lineTo(i, yMin); }
  }
  ctx.stroke();
}

async function resampleAudio(audioBuffer, targetDurationSec) {
  const actualDuration = Math.min(audioBuffer.duration, targetDurationSec);
  const offlineCtx = new OfflineAudioContext(1, actualDuration * TARGET_SAMPLE_RATE, TARGET_SAMPLE_RATE);
  const source = offlineCtx.createBufferSource();
  source.buffer = audioBuffer;
  source.connect(offlineCtx.destination);
  source.start(0, 0, actualDuration);
  const rendered = await offlineCtx.startRendering();
  return rendered.getChannelData(0);
}

// File Drag & Drop / Selection
dropZone.addEventListener('click', () => fileInput.click());
dropZone.addEventListener('dragover', e => { e.preventDefault(); dropZone.classList.add('dragover'); });
dropZone.addEventListener('dragleave', () => dropZone.classList.remove('dragover'));
dropZone.addEventListener('drop', e => {
  e.preventDefault();
  dropZone.classList.remove('dragover');
  if (e.dataTransfer.files.length) handleFile(e.dataTransfer.files[0]);
});
fileInput.addEventListener('change', e => {
  if (e.target.files.length) handleFile(e.target.files[0]);
});

async function handleFile(file) {
  fileName.textContent = file.name;
  try {
    const arrayBuffer = await file.arrayBuffer();
    if (!audioContext) audioContext = new (window.AudioContext || window.webkitAudioContext)();
    const audioBuffer = await audioContext.decodeAudioData(arrayBuffer);
    
    const t0 = performance.now();
    const float32Data = await resampleAudio(audioBuffer, 1.0);
    clientResampleTimeSec = ((performance.now() - t0) / 1000).toFixed(3);
    
    currentAudioPCM = floatTo16BitPCM(float32Data);
    drawWaveform(currentAudioPCM);
    btnClassify.disabled = false;
  } catch (e) {
    showToast('Failed to decode audio file. Please select a standard .wav');
    console.error(e);
  }
}

// Live Recording
let mediaStream = null, scriptProcessor = null, recordingTimeout = null, recordedChunks = [], recDuration = 1, isRecording = false;
durationBtns.forEach(btn => {
  btn.addEventListener('click', e => {
    if (isRecording) return;
    durationBtns.forEach(b => b.classList.remove('active'));
    e.target.classList.add('active');
    recDuration = parseInt(e.target.dataset.dur);
  });
});

async function startRecording() {
  if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
    showToast('Browser blocks live mic on HTTP. Opening audio recorder...');
    fileInput.click();
    return;
  }
  try {
    mediaStream = await navigator.mediaDevices.getUserMedia({ audio: true });
    if (!audioContext) audioContext = new (window.AudioContext || window.webkitAudioContext)();
    const source = audioContext.createMediaStreamSource(mediaStream);
    scriptProcessor = audioContext.createScriptProcessor(4096, 1, 1);
    recordedChunks = [];
    isRecording = true;
    btnRecord.textContent = '⏹️ Stop Recording';
    btnRecord.classList.add('danger');
    btnClassify.disabled = true;
    fileName.textContent = 'Recording live audio...';
    
    scriptProcessor.onaudioprocess = e => {
      recordedChunks.push(new Float32Array(e.inputBuffer.getChannelData(0)));
    };
    source.connect(scriptProcessor);
    scriptProcessor.connect(audioContext.destination);
    recordingTimeout = setTimeout(() => stopRecording(), recDuration * 1000);
  } catch (e) {
    showToast('Mobile browsers require HTTPS for live mic. Tap "Record/Upload Audio" above to record!');
    console.error(e);
  }
}

async function stopRecording() {
  if (!isRecording) return;
  isRecording = false;
  clearTimeout(recordingTimeout);
  if (scriptProcessor && mediaStream) {
    scriptProcessor.disconnect();
    mediaStream.getTracks().forEach(t => t.stop());
  }
  btnRecord.textContent = '🎤 Start Recording';
  btnRecord.classList.remove('danger');
  fileName.textContent = `Recorded ${recDuration}s audio clip`;

  const totalLen = recordedChunks.reduce((a, c) => a + c.length, 0);
  const merged = new Float32Array(totalLen);
  let offset = 0;
  for (const chunk of recordedChunks) {
    merged.set(chunk, offset);
    offset += chunk.length;
  }

  const t0 = performance.now();
  const offlineCtx = new OfflineAudioContext(1, (merged.length / audioContext.sampleRate) * TARGET_SAMPLE_RATE, TARGET_SAMPLE_RATE);
  const buffer = offlineCtx.createBuffer(1, merged.length, audioContext.sampleRate);
  buffer.copyToChannel(merged, 0);
  const src = offlineCtx.createBufferSource();
  src.buffer = buffer;
  src.connect(offlineCtx.destination);
  src.start(0);
  const rendered = await offlineCtx.startRendering();
  clientResampleTimeSec = ((performance.now() - t0) / 1000).toFixed(3);

  currentAudioPCM = floatTo16BitPCM(rendered.getChannelData(0));
  drawWaveform(currentAudioPCM);
  btnClassify.disabled = false;
}

btnRecord.addEventListener('click', () => {
  if (isRecording) stopRecording();
  else startRecording();
});

// Classification & Benchmark Timing
btnClassify.addEventListener('click', async () => {
  if (!currentAudioPCM) return;
  resultCard.classList.add('active');
  resultDetails.style.display = 'none';
  spinner.style.display = 'block';
  resultCard.scrollIntoView({ behavior: 'smooth' });

  const browserStartTime = performance.now();

  try {
    const response = await fetch('/classify', {
      method: 'POST',
      headers: { 'Content-Type': 'application/octet-stream' },
      body: currentAudioPCM.buffer
    });

    if (!response.ok) throw new Error('ESP32 returned error: ' + response.status);
    const data = await response.json();
    const browserEndTime = performance.now();
    const browserDurationSec = ((browserEndTime - browserStartTime) / 1000).toFixed(3);

    renderResults(data, browserDurationSec);
  } catch (err) {
    spinner.style.display = 'none';
    showToast('Classification error: ' + err.message);
    console.error(err);
  }
});

function renderResults(data, browserDurationSec) {
  spinner.style.display = 'none';
  resultDetails.style.display = 'block';

  // Prediction label and confidence
  const label = data.top_class || data.class || 'unknown';
  const emoji = emojiMap[label] || '';
  const conf = data.confidence !== undefined ? parseFloat(data.confidence).toFixed(2) : '0.00';

  document.getElementById('resPredName').textContent = (emoji ? emoji + ' ' : '') + label.replace('_', ' ');
  document.getElementById('resConfVal').textContent = conf + '%';

  // Timing breakdown from ESP32
  const timing = data.timing || {};
  document.getElementById('tUpload').textContent = (timing.upload_time_s !== undefined ? timing.upload_time_s.toFixed(3) : '0.000') + ' s';
  document.getElementById('tParse').textContent = (timing.wav_parse_time_s !== undefined ? timing.wav_parse_time_s.toFixed(3) : '0.001') + ' s';
  document.getElementById('tResample').textContent = (clientResampleTimeSec || '0.000') + ' s';
  document.getElementById('tMel').textContent = (timing.mel_spectrogram_time_s !== undefined ? timing.mel_spectrogram_time_s.toFixed(3) : ((data.preprocessing_time_ms || 0)/1000).toFixed(3)) + ' s';
  document.getElementById('tInf').textContent = (timing.inference_time_s !== undefined ? timing.inference_time_s.toFixed(3) : ((data.inference_time_ms || 0)/1000).toFixed(3)) + ' s';
  document.getElementById('tDeviceTotal').textContent = (timing.total_device_time_s !== undefined ? timing.total_device_time_s.toFixed(3) : '0.000') + ' s';

  // Browser timing
  document.getElementById('tBrowserTotal').textContent = browserDurationSec + ' s';

  // Top 5 list
  const top5List = document.getElementById('top5List');
  top5List.innerHTML = '';
  if (data.predictions && Array.isArray(data.predictions)) {
    data.predictions.slice(0, 5).forEach(p => {
      const pConf = parseFloat(p.conf || 0);
      const pEmoji = emojiMap[p.class] || '🔹';
      top5List.innerHTML += `
        <div class="pred-row">
          <div class="pred-name">${pEmoji} ${p.class.replace('_', ' ')}</div>
          <div class="pred-bar-bg"><div class="pred-bar" style="width: ${pConf}%"></div></div>
          <div class="pred-val">${pConf.toFixed(1)}%</div>
        </div>
      `;
    });
  }
}

btnUploadAnother.addEventListener('click', () => {
  resultCard.classList.remove('active');
  inputCard.scrollIntoView({ behavior: 'smooth' });
});

// Periodic health poll
async function fetchHealth() {
  try {
    const res = await fetch('/health');
    const data = await res.json();
    document.getElementById('connDot').className = 'status-dot connected';
    document.getElementById('connText').textContent = 'ESP32 Connected';
    document.getElementById('statInfo').textContent = `Free Heap: ${Math.round((data.free_heap || 0) / 1024)} KB`;
  } catch (e) {
    document.getElementById('connDot').className = 'status-dot disconnected';
    document.getElementById('connText').textContent = 'ESP32 Disconnected';
  }
}
setInterval(fetchHealth, 15000);
fetchHealth();

function resizeCanvas() {
  const r = canvas.parentElement.getBoundingClientRect();
  canvas.width = r.width - 24;
  canvas.height = 70;
  drawWaveform(currentAudioPCM);
}
window.addEventListener('resize', resizeCanvas);
resizeCanvas();
</script>
</body>
</html>
)rawliteral";
const int INDEX_HTML_LEN = sizeof(INDEX_HTML) - 1;

#endif
