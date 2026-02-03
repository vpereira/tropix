.he 'VIC (cmd)'TROPIX: Manual de Referência'VIC (cmd)'
.fo 'Atualizado em 03.02.26'Versão 1.0.0'Pag. %'
.bp

.b NOME
.in 5
.wo "vic -"
editor de texto visual (versão compacta do vi)
.br

.in
.sp
.b SINTAXE
.in 5
.(l
vic [-R] [-c <cmd>] [<arquivo> ...]
.)l

.in
.sp
.b DESCRIÇÃO
.in 5
O comando "vic" é um editor de texto visual, uma implementação
compacta do editor "vi" tradicional do UNIX.

.sp
O "vic" opera em dois modos principais:

.in +3
.ip "Modo COMANDO:"
O modo padrão, onde as teclas são interpretadas como comandos
de edição, navegação e manipulação de texto.

.ip "Modo INSERÇÃO:"
Neste modo, as teclas digitadas são inseridas diretamente no texto.
Para retornar ao modo comando, pressione <ESC>.

.ep
.in -3

.sp
As opções do comando são:

.in +3
.ip -R
Abre o arquivo em modo somente leitura.
Neste modo, o arquivo não pode ser modificado.

.ip "-c <cmd>"
Executa o comando "ex" especificado após carregar o arquivo.
Por exemplo: vic -c "10" arquivo (posiciona na linha 10).

.ep
.in -3

.sp 2
.b "COMANDOS DE NAVEGAÇÃO"
.in 5

.in +3
.ip "h, <BS>"
Move o cursor um caractere para a esquerda.

.ip "l, <SP>"
Move o cursor um caractere para a direita.

.ip "j, <NL>"
Move o cursor uma linha para baixo.

.ip "k"
Move o cursor uma linha para cima.

.ip "0"
Move o cursor para o início da linha.

.ip "$"
Move o cursor para o final da linha.

.ip "^"
Move o cursor para o primeiro caractere não-branco da linha.

.ip "G"
Move o cursor para a última linha do arquivo.

.ip "gg"
Move o cursor para a primeira linha do arquivo.

.ip "<CTRL>-F"
Avança uma tela (page down).

.ip "<CTRL>-B"
Retrocede uma tela (page up).

.ep
.in -3

.sp 2
.b "COMANDOS DE INSERÇÃO"
.in 5

.in +3
.ip "i"
Entra no modo de inserção antes do cursor.

.ip "a"
Entra no modo de inserção após o cursor.

.ip "I"
Entra no modo de inserção no início da linha.

.ip "A"
Entra no modo de inserção no final da linha.

.ip "o"
Abre uma nova linha abaixo e entra no modo de inserção.

.ip "O"
Abre uma nova linha acima e entra no modo de inserção.

.ip "R"
Entra no modo de substituição (sobrescreve caracteres).

.ip "<ESC>"
Retorna ao modo de comando.

.ep
.in -3

.sp 2
.b "COMANDOS DE DELEÇÃO"
.in 5

.in +3
.ip "x"
Apaga o caractere sob o cursor.

.ip "X"
Apaga o caractere antes do cursor.

.ip "dd"
Apaga a linha inteira.

.ip "D"
Apaga do cursor até o final da linha.

.ep
.in -3

.sp 2
.b "COMANDOS DE SUBSTITUIÇÃO"
.in 5

.in +3
.ip "r<c>"
Substitui o caractere sob o cursor pelo caractere <c>.

.ep
.in -3

.sp 2
.b "COMANDOS DE BUSCA"
.in 5

.in +3
.ip "/<padrão>"
Busca o <padrão> para frente no texto.

.ip "?<padrão>"
Busca o <padrão> para trás no texto.

.ip "n"
Repete a última busca na mesma direção.

.ip "N"
Repete a última busca na direção oposta.

.ep
.in -3

.sp 2
.b "COMANDOS EX (COLON)"
.in 5
Os comandos "ex" são iniciados com ":" no modo comando:

.in +3
.ip ":w [<arq>]"
Grava o arquivo. Se <arq> for especificado, grava com este nome.

.ip ":q"
Sai do editor. Falha se houver modificações não salvas.

.ip ":q!"
Sai do editor, descartando modificações não salvas.

.ip ":wq"
Grava e sai.

.ip ":x"
Grava (se modificado) e sai.

.ip ":e [<arq>]"
Edita outro arquivo. Se <arq> não for dado, recarrega o atual.

.ip ":e!"
Recarrega o arquivo, descartando modificações.

.ip ":r <arq>"
Insere o conteúdo de <arq> após a linha atual.

.ip ":<n>"
Vai para a linha <n>.

.ip ":!<cmd>"
Executa o comando <cmd> no shell.

.ep
.in -3

.sp 2
.b "OUTROS COMANDOS"
.in 5

.in +3
.ip "ZZ"
Grava (se modificado) e sai (equivalente a :x).

.ip "ZQ"
Sai sem gravar (equivalente a :q!).

.ip "<CTRL>-L"
Redesenha a tela.

.ep
.in -3

.in
.sp
.b "VARIÁVEIS DE AMBIENTE"
.in 5

.in +3
.ip "LINES"
Número de linhas do terminal (padrão: 24).

.ip "COLUMNS"
Número de colunas do terminal (padrão: 80).

.ep
.in -3

.in
.sp
.b OBSERVAÇÕES
.in 5
Esta é uma versão compacta do "vi", adequada para sistemas
com recursos limitados. Algumas funcionalidades avançadas
do "vi" original não estão disponíveis:

.sp
.in +3
- Comando "undo" múltiplo (apenas um nível)
.br
- Expressões regulares avançadas na busca
.br
- Macros e mapeamentos de teclas
.br
- Múltiplos buffers de edição
.br
- Redimensionamento dinâmico de janela
.in -3

.in
.sp
.b
VEJA TAMBÉM
.r
.in 5
.wo "(cmd): "
ed, cat, more
.br

.in
.sp
.b ARQUIVOS
.in 5
Não utiliza arquivos de configuração.

.in
.sp
.b ESTADO
.in 5
Efetivo.
.in
