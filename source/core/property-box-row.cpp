#include "application.hpp"
#include <iostream>

using namespace nanogui;

Application::PropertyBoxRow::PropertyBoxRow(
    Widget* window, const std::vector<Config::Property*> &properties, 
    const std::string &name, const std::string &unit, size_t precision, 
    float step, std::string tooltip, size_t total_width)
    : 
    properties(properties), last_values(properties.size()), float_boxes(properties.size())
{
    Widget* panel = window;
    if (!name.empty())
    {
        panel = new Widget(window);
        panel->set_layout(new GridLayout(Orientation::Horizontal, 1 + (int)properties.size(), Alignment::Fill));
        Label* label = new Label(panel, name, "sans-bold");
        label->set_fixed_width(86);
        label->set_tooltip(tooltip);
    }

    int width = (int)(total_width / properties.size());

    for (size_t i = 0; i < properties.size(); i++)
    {
        last_values[i] = *properties[i];

        float_boxes[i] = new FloatBox<float>(panel);
        float_boxes[i]->set_fixed_size({ width, 20 });
        float_boxes[i]->number_format("%.0" + std::to_string(precision) + "f");
        float_boxes[i]->set_editable(true);
        float_boxes[i]->set_value(properties[i]->getDisplay());
        float_boxes[i]->set_font_size(14);
        float_boxes[i]->set_units(unit);
        float_boxes[i]->set_spinnable(true);
        float_boxes[i]->set_value_increment(step);
        float_boxes[i]->set_tooltip(tooltip);

        float_boxes[i]->set_callback([float_box = float_boxes[i], prop = properties[i], precision](float value)
            {
                prop->setDisplay(value);
                float_box->set_value(*prop);
            }
        );
    }
}

void Application::PropertyBoxRow::updateValues()
{
    for (size_t i = 0; i < properties.size(); i++)
    {
        if (*properties[i] != last_values[i])
        {
            float_boxes[i]->set_value(properties[i]->getDisplay());
            last_values[i] = *properties[i];
        }
    }
}