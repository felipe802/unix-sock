CC       := gcc
CFLAGS   := -std=c23 -fstack-protector-strong
WFLAGS   := -Wformat=2 -Wall -Wextra -Wvla -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror -Wno-cpp -Wno-missing-field-initializers
CPPFLAGS := -I./inc -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=202405L -D_FORTIFY_SOURCE=2
LDFLAGS  := -flto
TARGET   := server

ifdef DEBUG
    CFLAGS += -O1 -g -fsanitize=address,undefined
else
    CFLAGS += -O2
endif

SRCDIR   := src
INCDIR   := inc
OBJDIR   := obj
DATADIR  := data

SOURCES  := $(wildcard $(SRCDIR)/*.c)
HEADERS  := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

.PHONY: all
all: build

.PHONY: build
build: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "🔗 Ligando objetos e gerando binário '$(TARGET)'..."
	@$(CC) $(CFLAGS) $(WFLAGS) $(LDFLAGS) $^ -o $@
	@echo "✅ Executável '$(TARGET)' Gerado com Sucesso!"

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@mkdir -p $(OBJDIR)
	@echo "⚙️  Compilando $<..."
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(WFLAGS) -c $< -o $@

.PHONY: run
run: build
	@echo "🟢 Iniciando Servidor..."
	@./$(TARGET)
	@echo "🛑 Servidor Encerrado."

.PHONY: clear clean
clear clean:
	@rm -rf $(OBJDIR) $(TARGET)
	@echo "🧹 Ambiente Limpo (Objetos e Binários Removidos)."

.PHONY: erase
erase: clean
	@rm -rf $(DATADIR)
	@echo "🔥 Apagamento Total! (Pasta de Dados também foi Destruída)."

.PHONY: clangd
clangd: clean
	bear -- $(MAKE) build
	@echo "📝 Arquivo 'compile_commands.json' atualizado para o Clangd!"
