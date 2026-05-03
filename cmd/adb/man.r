.bp
.he 'ADB (cmd)'TROPIX: Manual de Referência'ADB (cmd)'
.fo 'Atualizado em 26.01.26'Versão 1.0.0'Pag. %'

.b NOME
.in 5
.wo "adb -"
depurador simbólico
.br

.in
.sp
.b SINTAXE
.in 5
.(l
adb [-w] [<programa> [<core>]]
.)l

.in
.sp
.b DESCRIÇÃO
.in 5
O comando "adb" é um depurador simbólico para examinar
arquivos executáveis e arquivos core, e para controlar
a execução de programas.

.sp
O <programa> é normalmente um arquivo executável no formato a.out.
Se não for dado, "a.out" é usado por omissão.

.sp
O <core> é um arquivo de imagem de memória produzido após
uma falha do programa. Se não for dado, "core" é usado.

.sp
As opções são:

.in +3
.ip -w
Permite escrita no arquivo do programa.

.ip -H
Imprime ajuda e sai.
.in -3

.in
.sp
.b COMANDOS
.in 5

.in +3
.ip "<addr>?<fmt>" 15
Examina o conteúdo do arquivo no endereço <addr>.

.ip "<addr>/<fmt>" 15
Examina a memória do processo (ou core) no endereço <addr>.

.ip "<addr>:b" 15
Coloca um breakpoint no endereço <addr>.

.ip ":d" 15
Remove todos os breakpoints.

.ip ":r" 15
Inicia a execução do programa.

.ip ":c" 15
Continua a execução após parada.

.ip ":s" 15
Executa uma única instrução (single step).

.ip ":k" 15
Termina o processo em execução.

.ip "$r" 15
Mostra os registradores do processador.

.ip "$b" 15
Lista os breakpoints.

.ip "$m" 15
Mostra os mapas de memória.

.ip "$q" 15
Sai do depurador.
.in -3

.in
.sp
.b ENDEREÇOS
.in 5
Endereços podem ser especificados como:

.in +3
.ip "\(bu" 3
Números hexadecimais: 0x1234
.ip "\(bu" 3
Números decimais: 1234
.ip "\(bu" 3
Símbolos: main, printf
.ip "\(bu" 3
Símbolo+offset: main+0x10
.in -3

.in
.sp
.b
VEJA TAMBÉM
.r
.in 5
.wo "(cmd): "
cc, ld, nm
.br
.wo "(sys): "
ptrace
.br
.wo "(fmt): "
a.out, core
.br

.in
.sp
.b ESTADO
.in 5
Experimental.
.in
