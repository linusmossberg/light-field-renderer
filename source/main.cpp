#include <nanogui/nanogui.h>

#include <iostream>
#include <exception>

#include "core/application.hpp"

int main() 
{
    try 
    {
        nanogui::init();
        {
            nanogui::ref<Application> app = new Application();
            app->dec_ref();
            app->draw_all();
            app->set_visible(true);
            nanogui::mainloop(1000.0f / 60.f);
        }
        nanogui::shutdown();
    }
    catch (const std::exception &e) 
    {
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}