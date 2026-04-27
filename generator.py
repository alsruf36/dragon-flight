#!/usr/bin/env python3
import argparse
import json
from pathlib import Path
from typing import Iterable

from PIL import Image


try:
    RESAMPLE_LANCZOS = Image.Resampling.LANCZOS
except AttributeError:
    RESAMPLE_LANCZOS = Image.LANCZOS


ROOT_DIR = Path(__file__).resolve().parent
DEFAULT_VIDEO = ROOT_DIR / "assets" / "intro" / "intro.mp4"
DEFAULT_FRAME_DIR = ROOT_DIR / "tmp" / "intro_frames"
DEFAULT_ASCII_DIR = ROOT_DIR / "assets" / "intro"
DEFAULT_DATA_PATH = ROOT_DIR / "assets" / "intro" / "data.json"
DEFAULT_FRAME_COUNT = 82


def load_braille_data(data_path: Path) -> list[int]:
    with data_path.open("r", encoding="utf-8") as file:
        data = json.load(file)

    if isinstance(data, dict):
        data = data.get("brailleData", [])

    if not isinstance(data, list) or not data:
        raise ValueError(f"Invalid braille data in {data_path}")

    return [int(value) for value in data]


def get_threshold(px_array: Iterable[int]) -> int:
    px_list = list(px_array)
    if not px_list:
        raise ValueError("Pixel array is empty")
    return sum(px_list) // len(px_list)


def to_num(matrix: list[list[int]], chars_per_line: int, relative: int, negative: bool) -> list[int]:
    num_array = [0] * chars_per_line

    line_size = len(matrix)
    column_size = len(matrix[0])

    for i in range(line_size):
        for j in range(column_size):
            if negative:
                if matrix[i][j] < relative:
                    num_array[i // 2] = num_array[i // 2] + 2 ** ((j * 2) + (i % 2))
            else:
                if matrix[i][j] > relative:
                    num_array[i // 2] = num_array[i // 2] + 2 ** ((j * 2) + (i % 2))

    return num_array


def to_matrix(array: list[int], height_index: int, width: int) -> list[list[int]]:
    matrix = [[0 for _ in range(4)] for _ in range(width)]
    array_index = height_index * width
    for j in range(4):
        for i in range(width):
            matrix[i][j] = array[array_index]
            array_index = array_index + 1
    return matrix


def px_ascii_art(array: list[int], width: int, height: int, relative: int, negative: bool, braille_data: list[int]) -> str:
    width = width - width % 2
    height = height - height % 4
    number_of_lines = height // 4
    ascii_art = ""

    height_index = 0
    while number_of_lines > 0:
        matrix = to_matrix(array, height_index, width)
        num_array = to_num(matrix, width // 2, relative, negative)
        for number in num_array:
            ascii_art = ascii_art + chr(braille_data[number])

        ascii_art = ascii_art + "\n"
        height_index = height_index + 4
        number_of_lines = number_of_lines - 1

    return ascii_art


def resize_image(image: Image.Image, new_width: int) -> Image.Image:
    width, height = image.size
    new_height = int(new_width * (height / width))
    return image.resize((new_width, new_height), RESAMPLE_LANCZOS)


def image_to_ascii_art(
    image_path: Path,
    output_path: Path,
    braille_data: list[int],
    new_image_width: int = 330,
    threshold: int = 110,
    negative: bool = True,
) -> None:
    with Image.open(image_path) as image:
        image = resize_image(image, new_image_width)
        width, height = image.size

        gray_pixels = list(image.convert("L").tobytes())
        if threshold == -1:
            threshold = get_threshold(gray_pixels)

        ascii_art = px_ascii_art(gray_pixels, width, height, threshold, negative, braille_data)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(ascii_art, encoding="utf-8")


def extract_frames(video_path: Path, frame_dir: Path, fps: int) -> None:
    import subprocess

    if not video_path.exists():
        raise FileNotFoundError(f"Video not found: {video_path}")

    frame_dir.mkdir(parents=True, exist_ok=True)
    for old_frame in frame_dir.glob("*.jpg"):
        old_frame.unlink()

    command = [
        "ffmpeg",
        "-y",
        "-i",
        str(video_path),
        "-vf",
        f"fps={fps}",
        str(frame_dir / "%d.jpg"),
    ]
    subprocess.run(command, check=True)


def generate_ascii_from_frames(
    frame_dir: Path,
    ascii_dir: Path,
    data_path: Path,
    width: int,
    threshold: int,
    negative: bool,
    frame_limit: int | None,
) -> int:
    braille_data = load_braille_data(data_path)
    ascii_dir.mkdir(parents=True, exist_ok=True)

    for old_ascii in ascii_dir.glob("*.txt"):
        old_ascii.unlink()

    frame_paths = sorted(
        [path for path in frame_dir.glob("*.jpg") if path.stem.isdigit()],
        key=lambda path: int(path.stem),
    )
    if frame_limit is not None:
        frame_paths = [path for path in frame_paths if int(path.stem) <= frame_limit]
    if not frame_paths:
        raise FileNotFoundError(f"No JPG frames found in {frame_dir}")

    for frame_path in frame_paths:
        output_path = ascii_dir / f"{frame_path.stem}.txt"
        image_to_ascii_art(frame_path, output_path, braille_data, width, threshold, negative)
        print(f"generated {output_path.name}")

    return len(frame_paths)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate Dragon Flight ASCII intro assets")
    parser.add_argument("--video", type=Path, default=DEFAULT_VIDEO, help="Input intro video path")
    parser.add_argument("--frame-dir", type=Path, default=DEFAULT_FRAME_DIR, help="Extracted JPG frame directory")
    parser.add_argument("--ascii-dir", type=Path, default=DEFAULT_ASCII_DIR, help="Output ASCII frame directory")
    parser.add_argument("--data-path", type=Path, default=DEFAULT_DATA_PATH, help="Braille lookup JSON path")
    parser.add_argument("--fps", type=int, default=30, help="Frame extraction FPS")
    parser.add_argument("--width", type=int, default=330, help="ASCII resize width")
    parser.add_argument("--threshold", type=int, default=110, help="Luma threshold; use -1 for automatic")
    parser.add_argument("--negative", dest="negative", action="store_true", default=True, help="Use negative image thresholding")
    parser.add_argument("--positive", dest="negative", action="store_false", help="Use positive image thresholding")
    parser.add_argument("--skip-extract", action="store_true", help="Skip ffmpeg extraction and use existing JPG frames")
    parser.add_argument("--frame-limit", type=int, default=DEFAULT_FRAME_COUNT, help="Maximum numbered frame to convert")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if not args.skip_extract:
        extract_frames(args.video, args.frame_dir, args.fps)

    count = generate_ascii_from_frames(
        frame_dir=args.frame_dir,
        ascii_dir=args.ascii_dir,
        data_path=args.data_path,
        width=args.width,
        threshold=args.threshold,
        negative=args.negative,
        frame_limit=args.frame_limit,
    )
    print(f"generated {count} ASCII intro frames")


if __name__ == "__main__":
    main()
