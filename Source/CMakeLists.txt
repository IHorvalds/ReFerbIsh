project(ReFerbIsh VERSION 0.0.1)

set (BaseTargetName ReFerbIsh)

juce_add_plugin("${BaseTargetName}"
        VERSION "0.1"                         # Set this if the plugin version is different to the project version
        # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
        # ICON_SMALL ...
        COMPANY_NAME "IH Pedals"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE IHpd
        PLUGIN_CODE Rfrb
        FORMATS AU VST3 Standalone
        PRODUCT_NAME "ReFerbIsh Reverb")

juce_generate_juce_header(${BaseTargetName})

target_sources(${BaseTargetName} PRIVATE
        PluginProcessor.cpp
        PluginProcessor.h
        PluginEditor.cpp
        PluginEditor.h
        Parameters.h 
        Utils.cpp "Diffusion/Diffusion.h" "Diffusion/DiffusionStep.h" "Diffusion/DiffusionStep.cpp" "Feedback/Feedback.h")

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(${BaseTargetName}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_data_structures
        juce::juce_core
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_audio_processors
        juce::juce_audio_plugin_client
        juce::juce_audio_formats
        chowdsp::chowdsp_dsp_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)