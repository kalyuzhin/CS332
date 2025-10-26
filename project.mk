.PHONY: help
## prints help about all targets
help:
	@echo ""
	@echo "Usage:"
	@echo "  make <target>"
	@echo ""
	@echo "Targets:"
	@awk '                                \
		BEGIN { comment=""; }             \
		/^\s*##/ {                         \
		    comment = substr($$0, index($$0,$$2)); next; \
		}                                  \
		/^[a-zA-Z0-9_-]+:/ {               \
		    target = $$1;                  \
		    sub(":", "", target);          \
		    if (comment != "") {           \
		        printf "  %-17s %s\n", target, comment; \
		        comment="";                \
		    }                              \
		}' $(MAKEFILE_LIST)
	@echo ""

.PHONY: build
## builds project and return a.out file
build:
	g++ -std=c++20 main.cpp build/imgui/imgui_demo.cpp build/imgui/imgui_draw.cpp build/imgui/imgui.cpp build/imgui/imgui_widgets.cpp build/imgui/imgui_tables.cpp build/imgui/backends/imgui_impl_opengl3.cpp build/imgui/backends/imgui_impl_glfw.cpp -framework OpenGL `pkg-config --cflags --libs opencv4 glfw3` -lglfw -Ibuild/imgui -Ibuild/imgui/backends

.PHONY: run
## runs project
run: build
	$(shell ./a.out)