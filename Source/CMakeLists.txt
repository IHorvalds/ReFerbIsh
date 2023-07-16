juce_add_plugin("${CMAKE_PROJECT_NAME}"
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
        PRODUCT_NAME "ReFerbIsh Reverb MT - ${CMAKE_BUILD_TYPE}")

juce_generate_juce_header(${CMAKE_PROJECT_NAME})

add_subdirectory(Scheduler)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
        Scheduler)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        PluginProcessor.cpp
        PluginProcessor.h
        PluginEditor.cpp
        PluginEditor.h
        Parameters.h
        Utils.h

        Diffusion/Allpass.h
        Diffusion/Diffuser.h
        ParallelDiffusion/parallel_schroeder_allpass.h
        Reverb/Reverb.h
        )

target_compile_definitions(${CMAKE_PROJECT_NAME}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(${CMAKE_PROJECT_NAME}
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
        Microsoft.GSL::GSL
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)