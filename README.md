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

**Etapa 18 — Em desenvolvimento** 🚧

As etapas 1–18 já foram implementadas. O sistema de **configurações persistentes** restaura automaticamente o estado salvo do ícone ao abrir o Pingo, e a remoção do aplicativo apaga o arquivo de configuração correspondente.

A **Etapa 17** adicionou o painel **Configurar**, acessível pelo botão direito, com tamanho, posição fixa e modo de sobreposição. A **Etapa 18** consolidou os dois modos de janela: **Sempre sobre outras janelas** e **Modo normal**. A troca é aplicada imediatamente, sem precisar reiniciar o Pingo, e continua salva no `settings.json`.

## 🎯 Objetivo

Criar uma ferramenta pequena, rápida e prática para deixar os aplicativos favoritos acessíveis diretamente na tela, sem transformar o projeto em um launcher complexo.
