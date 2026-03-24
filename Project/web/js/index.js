document.addEventListener("DOMContentLoaded", carregarSalas);

function gerarIdSala() {
    return Math.random().toString(36).substring(2, 8).toUpperCase();
}

function carregarSalas() {
    fetch('/api/files')
        .then(res => res.json())
        .then(arquivos => {
            const roomList = document.getElementById('roomList');
            roomList.innerHTML = '';

            const salas = arquivos.filter(arq => arq.startsWith('sala_') && arq.endsWith('.json'));

            if (salas.length === 0) {
                roomList.innerHTML = '<div style="text-align: center; color: #94a3b8;">Nenhuma sala aberta. Crie uma!</div>';
                return;
            }

            salas.forEach(sala => {
                const idFormatado = sala.replace('sala_', '').replace('.json', '');

                const card = document.createElement('div');
                card.className = 'room-card';
                card.innerHTML = `
                    <div class="room-info">Sala: ${idFormatado}</div>
                    <button class="btn-join" onclick="entrarSala('${sala}')">Entrar 🎮</button>
                `;
                roomList.appendChild(card);
            });
        });
}

function criarSala() {
    const jogador = document.getElementById('playerName').value.trim();
    if (!jogador) {
        alert("Digite seu Nickname antes de criar uma sala!");
        return;
    }

    const idSala = gerarIdSala();
    const arquivoSala = `sala_${idSala}.json`;

    const estadoInicial = {
        board: ["", "", "", "", "", "", "", "", ""],
        turn: "X",
        players: { X: jogador, O: null },
        winner: null
    };

    fetch('/api/data/' + arquivoSala, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(estadoInicial)
    }).then(() => {
        window.location.href = `sala?sala=${arquivoSala}&jogador=${jogador}&simbolo=X`;
    });
}

function entrarSala(arquivoSala) {
    const jogador = document.getElementById('playerName').value.trim();
    if (!jogador) {
        alert("Digite seu Nickname antes de entrar!");
        return;
    }

    fetch('/api/data/' + arquivoSala)
        .then(res => res.json())
        .then(estadoJogo => {
            let simbolo = null;

            if (jogador === estadoJogo.players.X) {
                simbolo = 'X';
            }
            else if (jogador === estadoJogo.players.O) {
                simbolo = 'O';
            }
            else if (estadoJogo.players.O === null) {
                simbolo = 'O';
            }
            else {
                alert("Sala cheia! Os donos da sala são " + estadoJogo.players.X + " e " + estadoJogo.players.O);
                return;
            }

            window.location.href = `sala?sala=${arquivoSala}&jogador=${jogador}&simbolo=${simbolo}`;
        })
        .catch(err => {
            console.error(err);
            alert("Erro ao tentar ler os dados da sala.");
        });
}

setInterval(carregarSalas, 2048);
