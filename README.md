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

## 🛠️ Tecnologia

O projeto será desenvolvido em **C++**, utilizando recursos nativos do Windows para manter o aplicativo leve e simples.

## 📍 Status

**Etapa 15 — Em desenvolvimento** 🚧

As etapas 1–15 já foram implementadas. A **Etapa 14 — localizar arquivo** foi formalmente finalizada: o menu de contexto abre o Windows Explorer já selecionando o executável original do aplicativo. A **Etapa 15 — executar como administrador** utiliza o fluxo nativo de elevação do Windows (UAC) e trata falhas sem interromper o aplicativo.

A próxima etapa planejada é a **Etapa 16 — remover aplicativo**.

## 🎯 Objetivo

Criar uma ferramenta pequena, rápida e prática para deixar os aplicativos favoritos acessíveis diretamente na tela, sem transformar o projeto em um launcher complexo.
