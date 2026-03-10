document.getElementById('meuFormulario').addEventListener('submit', function(evento) {
    evento.preventDefault();

    const inputMensagem = document.getElementById('mensagem');
    const textoDigitado = inputMensagem.value;
    const boxResposta = document.getElementById('respostaServidor');
    const botao = document.querySelector('button');

    botao.textContent = 'Enviando...';
    botao.disabled = true;

    fetch('/api/data/mensagem.txt', {
        method: 'POST', 
        headers: {
            'Content-Type': 'text/plain'
        },
        body: textoDigitado
    })
    .then(resposta => resposta.text())
    .then(texto => {
        boxResposta.innerText = texto;
        boxResposta.className = 'resposta-visivel';
        
        inputMensagem.value = '';
        botao.textContent = 'Enviar via POST';
        botao.disabled = false;
    })
    .catch(erro => {
        console.error("Erro na comunicação:", erro);
        boxResposta.innerText = "Erro ao enviar dados para o servidor.";
        boxResposta.style.background = "#fee2e2";
        boxResposta.style.color = "#991b1b";
        boxResposta.className = 'resposta-visivel';
        
        botao.textContent = 'Enviar via POST';
        botao.disabled = false;
    });
});
