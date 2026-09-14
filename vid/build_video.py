#!/usr/bin/env python3
"""
Master Video Build Orchestrator: "Engineering the Billionth Prime"
Renders all 6 scenes with Manim, synchronizes narration audio, and concatenates
them into the final 10+ minute documentary video: final_billionth_prime_10min.mp4.
"""

import os
import sys
import json
import subprocess
import time

BASE_DIR = "/home/rishi/Desktop/tmp/1bgem/vid"
AUDIO_DIR = os.path.join(BASE_DIR, "audio")
OUTPUT_DIR = os.path.join(BASE_DIR, "output")
SCENES = [
    ("Act1Scene", "scene1", "Act I: The Frontier of Primes & The Billionth Target"),
    ("Act2Scene", "scene2", "Act II: The Physical Silicon & The CPU Memory Wall"),
    ("Act3Scene", "scene3", "Act III: The Architecture of the Segmented Sieve"),
    ("Act4Scene", "scene4", "Act IV: The Silicon Battlefield: x86_64 Assembly Optimization"),
    ("Act5Scene", "scene5", "Act V: The Vector Blitz: 512-Bit AVX-512 & BMI1"),
    ("Act6Scene", "scene6", "Act VI: Multi-Core Symphony & The Climax")
]

def run_cmd(cmd, desc):
    print(f"==> {desc}...")
    start = time.time()
    res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    elapsed = time.time() - start
    if res.returncode != 0:
        print(f"FAILED (code {res.returncode}): {res.stderr}")
        sys.exit(1)
    print(f"    Completed in {elapsed:.2f}s")
    return res

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    timings_path = os.path.join(AUDIO_DIR, "narration_timings.json")
    with open(timings_path, "r") as f:
        timings = json.load(f)

    manim_bin = os.path.join(BASE_DIR, "venv/bin/manim")
    rendered_acts = []

    for scene_class, scene_id, title in SCENES:
        print(f"\n=======================================================")
        print(f"PROCESSING: {title} ({scene_class})")
        print(f"=======================================================")
        
        # 1. Render Scene with Manim if not already rendered
        raw_video = os.path.join(BASE_DIR, f"media/videos/scenes/720p30/{scene_class}.mp4")
        if not os.path.exists(raw_video):
            cmd = f"{manim_bin} -qm --fps 30 {os.path.join(BASE_DIR, 'scenes.py')} {scene_class}"
            run_cmd(cmd, f"Rendering {scene_class} with Manim (720p30)")
        else:
            print(f"    {scene_class} already rendered at {raw_video}")

        # 2. Merge with narration audio
        audio_wav = timings[scene_id]["wav"]
        synced_act = os.path.join(OUTPUT_DIR, f"{scene_id}_synced.mp4")
        
        # Sync video and audio using ffmpeg
        # We ensure audio is clean, loud and clear, and stream-muxed
        mux_cmd = (
            f"ffmpeg -y -i '{raw_video}' -i '{audio_wav}' "
            f"-c:v copy -c:a aac -b:a 192k -shortest '{synced_act}'"
        )
        run_cmd(mux_cmd, f"Muxing {scene_id} audio and video")
        rendered_acts.append(synced_act)

    # 3. Concatenate all 6 acts into the final master video
    concat_list_path = os.path.join(OUTPUT_DIR, "concat_list.txt")
    with open(concat_list_path, "w") as f:
        for act in rendered_acts:
            f.write(f"file '{act}'\n")

    final_video = os.path.join(BASE_DIR, "final_billionth_prime_10min.mp4")
    print(f"\n=======================================================")
    print(f"CONCATENATING MASTER 10-MINUTE VIDEO...")
    print(f"=======================================================")
    concat_cmd = (
        f"ffmpeg -y -f concat -safe 0 -i '{concat_list_path}' "
        f"-c:v copy -c:a copy '{final_video}'"
    )
    run_cmd(concat_cmd, "Concatenating final master video")

    # 4. Probe final video duration
    probe = subprocess.run([
        "ffprobe", "-v", "error",
        "-show_entries", "format=duration,size,bit_rate",
        "-of", "json",
        final_video
    ], capture_output=True, text=True, check=True)
    info = json.loads(probe.stdout)["format"]
    total_dur = float(info["duration"])
    size_mb = float(info["size"]) / (1024 * 1024)

    # 5. Generate high-res cover thumbnail
    thumb_path = os.path.join(BASE_DIR, "video_thumbnail.png")
    thumb_cmd = f"ffmpeg -y -ss {total_dur - 15} -i '{final_video}' -vframes 1 -q:v 2 '{thumb_path}'"
    run_cmd(thumb_cmd, "Generating video thumbnail")

    print(f"\n*******************************************************")
    print(f"SUCCESS: Master video generated successfully!")
    print(f"Location: {final_video}")
    print(f"Duration: {total_dur:.2f} seconds ({total_dur/60:.2f} minutes)")
    print(f"File Size: {size_mb:.2f} MB")
    print(f"Thumbnail: {thumb_path}")
    print(f"*******************************************************\n")

if __name__ == "__main__":
    main()
