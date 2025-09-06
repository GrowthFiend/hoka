#include <FL/Fl.H>
#include <FL/Fl_Window.H>

int main(int argc, char **argv) {
    Fl_Window *window = new Fl_Window(400, 300, "Hoka");
    window->end();
    window->show();
    return Fl::run();
}