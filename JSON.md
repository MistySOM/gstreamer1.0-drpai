Here is a sample of JSON output when nothing is detected:

    {
        "video-rate": 33.3, 
        "drpai-rate": 2.5, 
        "detections": []
    }

Here is a sample of JSON output when the filters are **on**:

    {
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

Here is a sample of JSON output of Yolo models when the tracking is **off**:

    {
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

Here is a sample of JSON output of Yolo models when the tracking is **on**:

    {
        "video-rate": 32.3, 
        "drpai-rate": 2.5, 
        "detections": [
            {
                "id": 2, 
                "seen_first": "Fri Dec 22 23:20:56 2023", 
                "seen_last": "Fri Dec 22 23:21:46 2023", 
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
                "seen_first": "Fri Dec 22 23:25:53 2023", 
                "seen_last": "Fri Dec 22 23:26:07 2023", 
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
