# Pingo App

O **Pingo App** é um launcher flutuante para Windows que permite colocar aplicativos diretamente sobre a tela, com uma aparência inspirada nos ícones da Área de Trabalho.

A proposta é simples: arraste um aplicativo para o Pingo, deixe seu ícone flutuando onde quiser e abra-o rapidamente quando precisar — sem depender da Área de Trabalho.

## ✨ Proposta

- 📥 Adicionar aplicativos arrastando seus arquivos `.exe`
- 🖼️ Usar automaticamente o ícone do aplicativo
- 📝 Exibir e personalizar o nome do aplicativo
- 🪟 Manter os aplicativos como ícones flutuantes sobre a tela
- 🖱️ Mover os ícones livremente
- 🚀 Abrir aplicativos com duplo clique
- 📌 Permitir posições fixas
- 👻 Opção para manter os ícones sobre outras janelas
- ⚙️ Ações rápidas pelo botão direito

## 💾 Configurações persistentes

O Pingo salva o estado do ícone localmente em:

`%APPDATA%\Pingo App\settings.json`

São persistidos o caminho do `.exe`, nome personalizado, posição, tamanho, modo fixo, modo sempre sobre outras janelas e uma versão das configurações para futuras expansões.

## 🛠️ Tecnologia

O projeto será desenvolvido em **C++**, utilizando recursos nativos do Windows para manter o aplicativo leve e simples.

## 📍 Status

**Etapa 23 — Em desenvolvimento** 🚧

As etapas 1–22 já foram implementadas. A Etapa 23 também já possui implementação de suporte básico a múltiplos monitores. O sistema de **configurações persistentes** restaura automaticamente o estado salvo do ícone ao abrir o Pingo, e a remoção do aplicativo apaga o arquivo de configuração correspondente.

A **Etapa 17** adicionou o painel **Configurar**, acessível pelo botão direito, com tamanho, posição fixa e as opções de comportamento do ícone.

A **Etapa 18** implementou o modo **Sempre sobre outras janelas** como uma ação direta no menu do ícone. A opção aparece com marcação quando está ativa, pode ser ligada/desligada instantaneamente e a preferência continua salva no `settings.json`.

A **Etapa 19** implementou a opção **Não sobrepor outros aplicativos**. O comportamento padrão continua sendo manter o ícone sobre outras janelas; quando essa opção é ativada, o ícone deixa de ser TOPMOST e outras janelas podem ficar sobre ele. A preferência é salva no `settings.json`.

A **Etapa 20** adicionou **Posição fixa** diretamente ao menu do ícone. Quando ativada, o ícone não pode mais ser arrastado; quando desativada, o arraste volta imediatamente. O estado continua salvo no `settings.json`.

## 🎯 Objetivo

Criar uma ferramenta pequena, rápida e prática para deixar os aplicativos favoritos acessíveis diretamente na tela, sem transformar o projeto em um launcher complexo.


A **Etapa 21** foi validada no código existente: as alterações do ícone são gravadas automaticamente em `%APPDATA%\\Pingo App\\settings.json`, incluindo nome, posição, tamanho, posição fixa e preferência de sobreposição. O arquivo temporário é substituído de forma atômica para reduzir o risco de corromper as configurações.

A **Etapa 23** adicionou suporte básico a **múltiplos monitores**. O Pingo usa as áreas de trabalho dos monitores disponíveis para manter o ícone visível. Se um monitor for desconectado ou a configuração de telas mudar e a posição salva ficar fora de uma área disponível, o ícone é reposicionado automaticamente para uma área válida e a nova posição é salva.

A **Etapa 22** consolidou a **restauração das configurações** em uma rotina dedicada. Na inicialização, o Pingo recupera o aplicativo salvo, nome personalizado, posição, tamanho, posição fixa e preferência de sobreposição. O tamanho restaurado é validado no intervalo permitido antes da criação do layout, e a configuração normalizada é salva novamente. Se o executável salvo não existir mais, o item não é restaurado.