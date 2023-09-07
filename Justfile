run DIR:
    @bazel run --noshow_progress --ui_event_filters=-info,-stdout,-stderr //chopwatcher {{DIR}}
