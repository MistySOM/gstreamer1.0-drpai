Here is a sample of JSON output when nothing is detected:

```json
    {
        "timestamp": "2024-02-12T16:03:45.484Z",
        "video-rate": 33.3, 
        "drpai-rate": 2.5, 
        "detections": []
    }
```

Here is a sample of JSON output when the filters are **on**:

```json
    {
        "timestamp": "2024-02-12T16:03:45.484Z",
        "video-rate": 25.0, 
        "drpai-rate": 5.4, 
        "filter-classes": [
            "bottle"
        ], 
        "filter-region": {
            "left": 450, 
            "top": 350, 
            "width": 100, 
            "height": 100
        }, 
        "detections": []
    }
```

Here is a sample of JSON output of Yolo models when the tracking is **off**:

```json
    {
        "timestamp": "2024-02-12T16:03:45.484Z",
        "video-rate": 32.3, 
        "drpai-rate": 2.5, 
        "detections": [
            { 
                "class": "person", 
                "probability": 0.91, 
                "box": {
                    "centerX": 425, 
                    "centerY": 325, 
                    "width": 297, 
                    "height": 282
                }
            },
            {
                "class": "wine glass", 
                "probability": 0.52, 
                "box": {
                    "centerX": 87, 
                    "centerY": 133, 
                    "width": 122, 
                    "height": 172
                }
            }
        ]
    }
```

Here is a sample of JSON output of Yolo models when the tracking is **on**:

```json
    {
        "timestamp": "2024-02-12T16:03:45.484Z",
        "video-rate": 32.3, 
        "drpai-rate": 2.5, 
        "detections": [
            {
                "id": 2, 
                "seen_first": "2023-12-22T23:20:56.484Z", 
                "seen_last": "2023-12-22T23:21:46.484Z", 
                "class": "person", 
                "probability": 0.91, 
                "box": {
                    "centerX": 425, 
                    "centerY": 325, 
                    "width": 297, 
                    "height": 282
                }
            },
            {
                "id": 5, 
                "seen_first": "2023-12-22T23:25:53.484Z", 
                "seen_last": "2023-12-22T23:26:07.484Z", 
                "class": "wine glass", 
                "probability": 0.52, 
                "box": {
                    "centerX": 87, 
                    "centerY": 133, 
                    "width": 122, 
                    "height": 172
                }
            }
        ],
        "track-history": {
            "minutes": 60,
            "total-count": 6,
            "person": 2,
            "wine glass": 3,
            "chair": 1
        }
    }
```
