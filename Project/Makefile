CC       := gcc
CFLAGS   := -std=c23 -O2 -fstack-protector-strong
WFLAGS   := -Wformat=2 -Wall -Wextra -Wvla -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror
CPPFLAGS := -I./inc -D_POSIX_C_SOURCE=202405L -D_FORTIFY_SOURCE=2
LDFLAGS  :=
TARGET   := server

ifdef DEBUG
	CFLAGS += -O1 -g -fsanitize=address,undefined
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
	$(CC) $(CFLAGS) $(WFLAGS) $(LDFLAGS) $^ -o $@
	@echo "✅ Executável '$(TARGET)' Gerado com Sucesso!"

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WFLAGS) -c $< -o $@

.PHONY: run
run: build
	@echo "🚀 Iniciando Servidor..."
	@sudo ./$(TARGET)

.PHONY: clear clean
clear clean:
	@rm -rf $(OBJDIR) $(TARGET)
	@echo "🧹 Ambiente Limpo (Objetos e Binários Removidos)."

.PHONY: erase
erase: clean
	@sudo rm -rf $(DATADIR)
	@echo "🔥 Apagamento Total! (Pasta de Dados também foi Destruída)."
