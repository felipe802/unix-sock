const urlParams = new URLSearchParams(window.location.search);
const arquivoSala = urlParams.get('sala');
const jogador = urlParams.get('jogador');
const meuSimbolo = urlParams.get('simbolo');

document.getElementById('lblSala').textContent = arquivoSala.replace('sala_', '').replace('.json', '');
document.getElementById('lblJogador').textContent = jogador;
document.getElementById('lblSimbolo').textContent = meuSimbolo;

const statusBanner = document.getElementById('statusTurno');
const celulas = document.querySelectorAll('.cell');

let estadoJogo = null;
let atualizandoServidor = false;

function buscarEstadoServidor() {
    if (atualizandoServidor) return;
    fetch('/api/data/' + arquivoSala)
        .then(res => res.json())
        .then(data => {
            estadoJogo = data;
            if (meuSimbolo === 'O' && estadoJogo.players.O === null) {
                estadoJogo.players.O = jogador;
                atualizandoServidor = true;
                fetch('/api/data/' + arquivoSala, {
                    method: 'PUT',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(estadoJogo)
                }).finally(() => {
                    atualizandoServidor = false;
                });
            }
            atualizarInterface();
        })
        .catch(err => console.error("Sala excluída ou erro de conexão.", err));
}

function atualizarInterface() {
    if (!estadoJogo) return;

    for (let i = 0; i < 9; i++) {
        celulas[i].textContent = estadoJogo.board[i];
        celulas[i].className = `cell ${estadoJogo.board[i]}`;
    }

    if (estadoJogo.winner) {
        statusBanner.className = "status-banner fim-jogo";
        if (estadoJogo.winner === "Empate") {
            statusBanner.textContent = "Deu Velha! (Empate)";
        } else {
            statusBanner.textContent = `Vitória do ${estadoJogo.winner}! 🎉`;
        }
        return;
    }

    if (estadoJogo.turn === meuSimbolo) {
        statusBanner.textContent = "Sua vez de jogar!";
        statusBanner.className = "status-banner vez-ativa";
    } else {
        statusBanner.textContent = `Aguardando oponente (${estadoJogo.turn})...`;
        statusBanner.className = "status-banner vez-espera";
    }
}

function jogar(indice) {
    if (!estadoJogo || estadoJogo.winner) return;
    if (estadoJogo.turn !== meuSimbolo) {
        alert("Não é a sua vez!");
        return;
    }
    if (estadoJogo.board[indice] !== "") {
        return;
    }

    estadoJogo.board[indice] = meuSimbolo;
    verificarVencedor();

    if (!estadoJogo.winner && !estadoJogo.board.includes("")) {
        estadoJogo.winner = "Empate";
    } else if (!estadoJogo.winner) {
        estadoJogo.turn = meuSimbolo === 'X' ? 'O' : 'X';
    }

    atualizarInterface();

    atualizandoServidor = true;
    fetch('/api/data/' + arquivoSala, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(estadoJogo)
    }).finally(() => {
        atualizandoServidor = false;
    });
}

function verificarVencedor() {
    const vitorias = [
        [0, 1, 2], [3, 4, 5], [6, 7, 8],
        [0, 3, 6], [1, 4, 7], [2, 5, 8],
        [0, 4, 8], [2, 4, 6]
    ];

    for (let combo of vitorias) {
        const [a, b, c] = combo;
        if (estadoJogo.board[a] &&
            estadoJogo.board[a] === estadoJogo.board[b] &&
            estadoJogo.board[a] === estadoJogo.board[c]) {
            estadoJogo.winner = estadoJogo.board[a];
            return;
        }
    }
}

function voltarParaLobby() {
    window.location.href = "/";
}

setInterval(buscarEstadoServidor, 1024);
buscarEstadoServidor();
