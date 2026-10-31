const picker = document.querySelector("#pkg");
const info = document.querySelector("#file-info");
const upload = document.querySelector("#upload");
const status = document.querySelector("#status");

picker.addEventListener("change", () => {
  const file = picker.files && picker.files[0];
  if (!file) {
    info.textContent = "No package selected";
    upload.disabled = true;
    return;
  }
  info.textContent = file.name + " · " + (file.size / 1024 / 1024).toFixed(1) + " MB";
  upload.disabled = !file.name.toLowerCase().endsWith(".pkg");
  status.textContent = upload.disabled
    ? "Choose a file with the .pkg extension."
    : "Package selected. PS4 bridge is not connected; transfer is disabled.";
});

upload.addEventListener("click", () => {
  status.textContent = "Transfer unavailable: the PS4 HTTP bridge has not been implemented.";
});
