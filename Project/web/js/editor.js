const fileList = document.getElementById('fileList');
const editor = document.getElementById('meuEditor');
const currentFileSpan = document.getElementById('currentFile');
const btnSalvar = document.getElementById('btnSalvar');
const btnNovo = document.getElementById('btnNovo');
const inputNovo = document.getElementById('newFileName');
const statusMsg = document.getElementById('statusMensagem');

let arquivoAtual = null;

window.onload = carregarListaArquivos;

function carregarListaArquivos() {
    fetch('/api/files')
        .then(res => res.json())
        .then(arquivos => {
            fileList.innerHTML = '';
            if (arquivos.length === 0) {
                fileList.innerHTML = '<li>Nenhum arquivo encontrado</li>';
                return;
            }
            arquivos.forEach(arq => {
                const li = document.createElement('li');
                li.textContent = arq;
                if(arq === arquivoAtual) li.className = 'ativo';
                li.onclick = () => abrirArquivo(arq);
                fileList.appendChild(li);
            });
        });
}

function abrirArquivo(nomeArquivo) {
    arquivoAtual = nomeArquivo;
    currentFileSpan.textContent = nomeArquivo;
    
    editor.disabled = false;
    btnSalvar.disabled = false;
    editor.placeholder = "Carregando...";

    fetch('/api/data/' + nomeArquivo)
        .then(res => res.ok ? res.text() : "")
        .then(texto => {
            editor.value = texto;
            editor.placeholder = "Digite aqui...";
            carregarListaArquivos();
        });
}

btnNovo.onclick = () => {
    let nome = inputNovo.value.trim();
    if(!nome) return;
    if(!nome.includes('.')) nome += '.txt';
    
    fetch('/api/data/' + nome, { method: 'POST', body: "" })
        .then(() => {
            inputNovo.value = '';
            abrirArquivo(nome);
        });
};

btnSalvar.onclick = () => {
    if (!arquivoAtual) return;
    
    btnSalvar.textContent = 'Salvando...';
    btnSalvar.disabled = true;

    fetch('/api/data/' + arquivoAtual, {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: editor.value
    })
    .then(res => {
        if(res.ok) mostrarStatus("Salvo com sucesso!", "sucesso");
        else mostrarStatus("Erro ao salvar.", "erro");
    })
    .finally(() => {
        btnSalvar.textContent = 'Salvar Arquivo';
        btnSalvar.disabled = false;
    });
};

function mostrarStatus(msg, classe) {
    statusMsg.textContent = msg;
    statusMsg.className = `status-visivel ${classe}`;
    setTimeout(() => { statusMsg.className = 'status-oculto'; }, 3000);
}
