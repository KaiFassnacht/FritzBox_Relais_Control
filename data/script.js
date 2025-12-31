document.addEventListener('DOMContentLoaded', function() {
    // Vorhandene Formular-Logik (unverändert)
    const form = document.querySelector('form');
    if (form) {
        form.onsubmit = function(e) {
            e.preventDefault();
            const formData = new FormData(this);
            const statusText = document.getElementById('status-msg');
            const action = this.getAttribute('action');
            
            handleFetch(action, formData, statusText);
        };
    }
});

// Hilfsfunktion für Fetch-Requests
function handleFetch(action, body, statusText) {
    if (statusText) {
        statusText.style.display = 'block';
        statusText.style.background = '#e2f3ff';
        statusText.style.color = '#004085';
        statusText.innerHTML = "Verarbeite... Bitte warten.";
    }

    fetch(action, { method: 'POST', body: body })
    .then(response => {
        if (response.ok) {
            if (action === '/save' || action === '/upload-config') {
                let seconds = 5;
                const timer = setInterval(() => {
                    statusText.innerHTML = `Erfolgreich! Neustart erfolgt... Rückkehr in ${seconds} Sek.`;
                    seconds--;
                    if (seconds < 0) {
                        clearInterval(timer);
                        window.location.href = '/';
                    }
                }, 1000);
            } else {
                statusText.innerHTML = "Einstellungen live übernommen!";
                statusText.style.background = "#d4edda";
                statusText.style.color = "green";
                setTimeout(() => { statusText.style.display = 'none'; }, 3000);
            }
        } else {
            statusText.innerHTML = "Fehler bei der Verarbeitung!";
            statusText.style.color = "red";
        }
    })
    .catch(error => {
        statusText.innerHTML = "Verbindung verloren (Neustart läuft...)";
        setTimeout(() => window.location.href = '/', 5000);
    });
}

// Neue globale Funktion für den Datei-Upload
function uploadConfig() {
    const fileInput = document.getElementById('configFile');
    const statusText = document.getElementById('status-msg');
    
    if (fileInput.files.length === 0) return;

    const formData = new FormData();
    formData.append("file", fileInput.files[0]);

    handleFetch('/upload-config', formData, statusText);
}