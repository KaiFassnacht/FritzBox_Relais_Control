document.addEventListener('DOMContentLoaded', function() {
    const form = document.querySelector('form');
    if (!form) return;

    form.onsubmit = function(e) {
        e.preventDefault();
        
        const formData = new FormData(this);
        const statusText = document.getElementById('status-msg');
        const action = this.getAttribute('action'); // Holt /save oder /save-audio
        
        if (statusText) {
            statusText.style.display = 'block';
            statusText.style.color = '#004085';
            statusText.innerHTML = "Speichere... Bitte warten.";
        }

        fetch(action, {
            method: 'POST',
            body: formData
        })
        .then(response => {
            if (response.ok) {
                if (action === '/save') {
                    // Logik für Neustart
                    let seconds = 5;
                    const timer = setInterval(() => {
                        statusText.innerHTML = `Gespeichert! Neustart erfolgt... Rückkehr zum Menü in ${seconds} Sek.`;
                        seconds--;
                        if (seconds < 0) {
                            clearInterval(timer);
                            window.location.href = '/';
                        }
                    }, 1000);
                } else {
                    // Logik für Audio (ohne Neustart)
                    statusText.innerHTML = "Audio-Einstellungen live übernommen!";
                    statusText.style.color = "green";
                    setTimeout(() => { statusText.style.display = 'none'; }, 3000);
                }
            } else {
                statusText.innerHTML = "Fehler beim Speichern!";
                statusText.style.color = "red";
            }
        })
        .catch(error => {
            statusText.innerHTML = "Verbindung verloren (Neustart läuft vermutlich...)";
            setTimeout(() => window.location.href = '/', 5000);
        });
    };
});