const fileInput = document.getElementById("csvFile");
const instructionFilter = document.getElementById("instructionFilter");
const stageFilter = document.getElementById("stageFilter");
const summary = document.getElementById("summary");
const timeline = document.getElementById("timeline");

let rows = [];

function parseCsv(text) {
  const lines = text.trim().split(/\r?\n/);
  const header = lines.shift().split(",");
  return lines.filter(Boolean).map((line) => {
    const cells = line.split(",");
    const row = {};
    header.forEach((name, index) => row[name] = cells[index] || "");
    return row;
  });
}

function updateStageOptions() {
  const stages = [...new Set(rows.map((row) => row.stage).filter(Boolean))].sort();
  stageFilter.innerHTML = '<option value="">All stages</option>' +
    stages.map((stage) => `<option value="${stage}">${stage}</option>`).join("");
}

function render() {
  const instructionText = instructionFilter.value.trim();
  const stage = stageFilter.value;
  const filtered = rows.filter((row) => {
    if (instructionText && row.instruction_id !== instructionText) return false;
    if (stage && row.stage !== stage) return false;
    return true;
  });

  const cycles = [...new Set(filtered.map((row) => Number(row.cycle)))].sort((a, b) => a - b);
  const ids = [...new Set(filtered.map((row) => row.instruction_id))].sort((a, b) => Number(a) - Number(b));
  const byKey = new Map();
  for (const row of filtered) {
    const key = `${row.instruction_id}:${row.cycle}`;
    if (!byKey.has(key)) byKey.set(key, []);
    byKey.get(key).push(row);
  }

  summary.textContent = `${filtered.length} events, ${ids.length} instructions, ${cycles.length} cycles`;
  if (filtered.length === 0) {
    timeline.innerHTML = "";
    return;
  }

  let html = "<table><thead><tr><th>Instruction</th>";
  for (const cycle of cycles) html += `<th>${cycle}</th>`;
  html += "</tr></thead><tbody>";
  for (const id of ids) {
    const first = filtered.find((row) => row.instruction_id === id);
    html += `<tr><td>#${id} pc=${first.pc}<br>${first.instruction}</td>`;
    for (const cycle of cycles) {
      const events = byKey.get(`${id}:${cycle}`) || [];
      html += "<td>" + events.map((row) =>
        `<span class="event ${row.event}" title="${row.stage} ROB ${row.rob_index}">${row.event}</span>`
      ).join("<br>") + "</td>";
    }
    html += "</tr>";
  }
  html += "</tbody></table>";
  timeline.innerHTML = html;
}

fileInput.addEventListener("change", async () => {
  const file = fileInput.files[0];
  if (!file) return;
  rows = parseCsv(await file.text());
  updateStageOptions();
  render();
});

instructionFilter.addEventListener("input", render);
stageFilter.addEventListener("change", render);
