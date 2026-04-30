const form = document.querySelector("#run-form");
const graph = document.querySelector("#graph");
const kripkeView = document.querySelector("#kripke-view");
const logView = document.querySelector("#log-view");
const result = document.querySelector("#result");
const pathValue = document.querySelector("#path");
const runtime = document.querySelector("#runtime");
const entries = document.querySelector("#entries");
const button = form.querySelector("button");

function paramsFromForm() {
  const data = new FormData(form);
  return new URLSearchParams({
    state_count: data.get("state_count"),
    candidate_count: data.get("candidate_count"),
    voter_count: data.get("voter_count"),
    agent_count: data.get("agent_count"),
  });
}

function setLoading(isLoading) {
  button.disabled = isLoading;
  button.textContent = isLoading ? "Running..." : "Run ProbeMachine";
}

function escapeHtml(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function renderPillList(items, variant = "") {
  return items.map((item) => `<span class="pill ${variant}">${escapeHtml(item)}</span>`).join("");
}

function renderTripletTable(items, limit = 12) {
  const visible = items.slice(0, limit);
  if (!visible.length) {
    return `<div class="muted-note">No entries parsed.</div>`;
  }
  return `
    <table>
      <thead><tr><th>Source</th><th>Predicate</th><th>Target</th></tr></thead>
      <tbody>
        ${visible
          .map(
            (item) => `
              <tr>
                <td>${escapeHtml(item.source)}</td>
                <td>${escapeHtml(item.predicate)}</td>
                <td>${escapeHtml(item.target)}</td>
              </tr>`
          )
          .join("")}
      </tbody>
    </table>
    ${items.length > limit ? `<div class="muted-note">Showing ${limit} of ${items.length} entries.</div>` : ""}
  `;
}

function renderKripke(kripkeJson) {
  const kripke = JSON.parse(kripkeJson);
  const transitions = kripke.transitions || [];
  const labels = kripke.propLabels || {};
  const labelRows = Object.entries(labels)
    .map(([state, values]) => {
      const names = Object.keys(values || {});
      return `
        <article class="label-row">
          <strong>${escapeHtml(state)}</strong>
          <div>${renderPillList(names)}</div>
        </article>`;
    })
    .join("");

  kripkeView.classList.remove("empty-view");
  kripkeView.innerHTML = `
    <div class="artifact-stats">
      <article><span>States</span><strong>${kripke.states.length}</strong></article>
      <article><span>Transitions</span><strong>${transitions.length}</strong></article>
      <article><span>Initial</span><strong>${escapeHtml(kripke.initialStates.join(", "))}</strong></article>
    </div>

    <section class="subsection">
      <h3>State Set</h3>
      <div class="pill-cloud">${renderPillList(kripke.states, "state-pill")}</div>
    </section>

    <section class="subsection">
      <h3>Transition Relation</h3>
      <div class="transition-list">
        ${transitions
          .map((transition) => `<span>${escapeHtml(transition.from)} <b>→</b> ${escapeHtml(transition.to)}</span>`)
          .join("")}
      </div>
    </section>

    <section class="subsection">
      <h3>Label Function</h3>
      <div class="label-grid">${labelRows}</div>
    </section>

    <details>
      <summary>Raw kripke.json</summary>
      <pre>${escapeHtml(kripkeJson)}</pre>
    </details>
  `;
}

function renderCase(caseResult) {
  const summary = caseResult.summary || {};
  result.textContent = summary.result || "Unknown";
  result.style.color = summary.result === "True" ? "var(--gold)" : "var(--red)";
  pathValue.textContent = summary.path || "-";
  runtime.textContent = summary.seconds == null ? "-" : `${summary.seconds.toFixed(3)}s`;
  entries.textContent = summary.dataEntryCount ?? "-";

  if (caseResult.ok) {
    logView.classList.remove("error", "empty-view");
    logView.innerHTML = renderStructuredCase(summary, caseResult.stdout);
    return;
  }

  logView.classList.add("error");
  logView.textContent =
    caseResult.error || "case.cpp could not be compiled or executed. Check whether g++ and nlohmann/json are installed.";
}

function renderStructuredCase(summary, stdout) {
  const parameters = summary.parameters || {};
  const probes = summary.probeLibrary || [];
  const steps = summary.steps || [];
  return `
    <div class="artifact-stats">
      <article><span>Voters</span><strong>${parameters.voters ?? "-"}</strong></article>
      <article><span>Candidates</span><strong>${parameters.candidates ?? "-"}</strong></article>
      <article><span>Agents</span><strong>${parameters.agents ?? "-"}</strong></article>
    </div>

    <section class="subsection">
      <h3>Data Library X</h3>
      ${renderTripletTable(summary.dataLibrary || [], 10)}
    </section>

    <section class="subsection">
      <h3>Probe Library Y</h3>
      <div class="probe-grid">
        ${
          probes.length
            ? probes.slice(0, 18).map((probe) => `<span>${escapeHtml(probe.from)} <b>↦</b> ${escapeHtml(probe.to)}</span>`).join("")
            : `<div class="muted-note">No probes parsed.</div>`
        }
      </div>
      ${probes.length > 18 ? `<div class="muted-note">Showing 18 of ${probes.length} probes.</div>` : ""}
    </section>

    <section class="subsection">
      <h3>Probe Steps</h3>
      <div class="step-stack">
        ${steps.map((step) => renderStep(step)).join("") || `<div class="muted-note">No steps parsed.</div>`}
      </div>
    </section>

    <details>
      <summary>Raw case.cpp log</summary>
      <pre>${escapeHtml(stdout || "")}</pre>
    </details>
  `;
}

function renderStep(step) {
  return `
    <article class="step-card">
      <header>
        <strong>${escapeHtml(step.name)}</strong>
        <span>${step.items.length} aggregates</span>
      </header>
      ${renderTripletTable(step.items, 6)}
    </article>
  `;
}

async function runDemo() {
  setLoading(true);
  logView.classList.remove("error");
  logView.classList.add("empty-view");
  logView.textContent = "Running C++ case and generating artifacts...";
  try {
    const response = await fetch(`/api/run?${paramsFromForm().toString()}`);
    const payload = await response.json();
    graph.classList.remove("graph-empty");
    graph.innerHTML = payload.artifacts["graph.svg"];
    renderKripke(payload.artifacts["kripke.json"]);
    renderCase(payload.case);
  } catch (error) {
    logView.classList.add("error");
    logView.textContent = `Demo server error: ${error}`;
  } finally {
    setLoading(false);
  }
}

form.addEventListener("submit", (event) => {
  event.preventDefault();
  runDemo();
});

runDemo();
