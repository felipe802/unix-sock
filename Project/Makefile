CC       := clang
CFLAGS   := -Wall -Wextra -Werror -std=c23 -O2
CPPFLAGS := -I./inc -D_POSIX_C_SOURCE=202405L
TARGET   := server

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
	$(CC) $(CFLAGS) $^ -o $@
	@echo "✅ Executável '$(TARGET)' Gerado com Sucesso!"

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: run
run: build
	@echo "🚀 Iniciando Servidor..."
	./$(TARGET)

.PHONY: clear clean
clear clean:
	@rm -rf $(OBJDIR) $(TARGET)
	@echo "🧹 Ambiente Limpo (Objetos e Binários Removidos)."

.PHONY: erase
erase: clean
	@rm -rf $(DATADIR)
	@echo "🔥 Apagamento Total! (Pasta de Dados também foi Destruída)."
