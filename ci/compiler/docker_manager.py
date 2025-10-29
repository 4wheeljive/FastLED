"""Docker management for FastLED compilation.

This module provides clean abstraction layers for Docker operations including
container lifecycle management, image building, and compilation orchestration.
"""

import os
import subprocess
import sys
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import List, Optional

from ci.util.docker_helper import get_docker_command


class ContainerState(Enum):
    """Docker container states."""

    NOT_EXISTS = "not_exists"
    STOPPED = "stopped"
    PAUSED = "paused"
    RUNNING = "running"


@dataclass(frozen=True)
class DockerConfig:
    """Immutable Docker configuration."""

    board_name: str
    image_name: str
    project_root: Path
    container_name: str
    mount_target: str = "//host"  # Git-bash compatible path
    stop_timeout: int = 300
    output_dir: Optional[Path] = None  # Optional host directory for build artifacts

    @staticmethod
    def _validate_output_dir(output_dir: Path, project_root: Path) -> Path:
        """Validate and prepare output directory.

        Args:
            output_dir: Path to output directory (can be relative or absolute)
            project_root: Project root path for validation

        Returns:
            Absolute path to validated output directory

        Raises:
            ValueError: If output directory is outside project root or parent directory
        """
        # Resolve to absolute path
        if not output_dir.is_absolute():
            output_dir = (project_root / output_dir).resolve()
        else:
            output_dir = output_dir.resolve()

        # Security check: only allow directories within or relative to project root
        try:
            # Check if output_dir is relative to project_root or current working directory
            cwd = Path.cwd().resolve()
            try:
                output_dir.relative_to(project_root)
                # Output dir is under project root - OK
            except ValueError:
                # Not under project root, check if under current working directory
                output_dir.relative_to(cwd)
                # Output dir is under cwd - OK
        except ValueError:
            raise ValueError(
                f"Output directory must be within project root ({project_root}) "
                f"or current working directory ({cwd}), got: {output_dir}"
            )

        # Create directory if it doesn't exist
        output_dir.mkdir(parents=True, exist_ok=True)

        return output_dir

    @classmethod
    def from_board(
        cls, board_name: str, project_root: Path, output_dir: Optional[Path] = None
    ) -> "DockerConfig":
        """Factory method to create config from board name.

        Maps boards to their platform family Docker images and containers.
        Each platform family (AVR, ESP, Teensy, etc.) has ONE Docker image that
        contains pre-cached toolchains for ALL boards in that family.

        All platform images use the :latest tag since they contain all necessary
        toolchains for all boards in the platform family.

        Examples (board -> image, container):
            uno -> niteris/fastled-compiler-avr:latest, container: fastled-compiler-avr
            esp32s3 -> niteris/fastled-compiler-esp-xtensa:latest, container: fastled-compiler-esp-xtensa
            esp32c3 -> niteris/fastled-compiler-esp-riscv:latest, container: fastled-compiler-esp-riscv
            teensy41 -> niteris/fastled-compiler-teensy:latest, container: fastled-compiler-teensy
        """
        from ci.docker.build_platforms import (
            get_docker_image_name,
            get_platform_for_board,
        )

        try:
            # Look up which platform family this board belongs to
            platform = get_platform_for_board(board_name)

            if platform:
                # Get the platform family image name (e.g., niteris/fastled-compiler-esp-esp32dev)
                # This returns the full image name with registry prefix and representative board
                platform_image = get_docker_image_name(platform)
                # Keep the full registry prefix (niteris/) for Docker Hub compatibility
                # Docker will automatically pull from the registry if image is not found locally
                image_name = f"{platform_image}:latest"
            else:
                # Board not in platform mapping - generate ad-hoc name with registry prefix
                from ci.docker.build_image import extract_architecture

                arch = extract_architecture(board_name)
                image_name = f"niteris/fastled-compiler-{arch}-{board_name}:latest"

        except Exception as e:
            # Fallback if lookups fail
            print(
                f"Warning: Could not map board to platform image for {board_name}: {e}"
            )
            image_name = f"niteris/fastled-compiler-{board_name}:latest"

        # Container name uses platform-level naming (one container per platform family)
        # e.g., fastled-compiler-avr, fastled-compiler-esp-xtensa, etc.
        # This matches the Docker image strategy and allows sharing containers across boards
        if platform:
            container_name = f"fastled-compiler-{platform}"
        else:
            # Fallback for unmapped boards
            container_name = f"fastled-compiler-{board_name}"

        # Use git-bash compatible mount target on Windows
        mount_target = "//host" if sys.platform == "win32" else "/host"

        # Validate and resolve output directory if provided
        validated_output_dir = None
        if output_dir is not None:
            validated_output_dir = cls._validate_output_dir(output_dir, project_root)

        return cls(
            board_name=board_name,
            image_name=image_name,
            project_root=project_root,
            container_name=container_name,
            mount_target=mount_target,
            output_dir=validated_output_dir,
        )


class DockerContainerManager:
    """Manages Docker container lifecycle with minimal side effects."""

    def __init__(self, config: DockerConfig):
        self.config = config
        self._image_name_override: Optional[str] = None

    def set_image_name(self, image_name: str) -> None:
        """Override the image name to use (for cases where local image differs from config)."""
        self._image_name_override = image_name

    def _get_image_name(self) -> str:
        """Get the image name to use for this container."""
        return self._image_name_override or self.config.image_name

    def get_container_state(self) -> ContainerState:
        """Query current container state without side effects."""
        try:
            # Check if container exists
            result = subprocess.run(
                [
                    get_docker_command(),
                    "container",
                    "inspect",
                    self.config.container_name,
                ],
                capture_output=True,
                text=True,
                timeout=10,
            )
            if result.returncode != 0:
                return ContainerState.NOT_EXISTS

            # Parse state from output
            state_result = subprocess.run(
                [
                    get_docker_command(),
                    "inspect",
                    "-f",
                    "{{.State.Running}} {{.State.Paused}}",
                    self.config.container_name,
                ],
                capture_output=True,
                text=True,
                timeout=10,
            )
            parts = state_result.stdout.strip().split()
            is_running = parts[0] == "true"
            is_paused = parts[1] == "true" if len(parts) > 1 else False

            if is_paused:
                return ContainerState.PAUSED
            elif is_running:
                return ContainerState.RUNNING
            else:
                return ContainerState.STOPPED
        except (FileNotFoundError, subprocess.TimeoutExpired):
            # If Docker is not available, we can't check state
            return ContainerState.NOT_EXISTS

    def create_container(self) -> bool:
        """Create a new container."""
        env = os.environ.copy()
        env["MSYS_NO_PATHCONV"] = "1"

        cmd = [
            get_docker_command(),
            "create",
            "--name",
            self.config.container_name,
            "-v",
            f"{self.config.project_root}:{self.config.mount_target}:ro",
        ]

        # Add optional output directory mount
        if self.config.output_dir is not None:
            cmd.extend(["-v", f"{self.config.output_dir}:/fastled/output:rw"])

        cmd.extend(
            [
                "--stop-timeout",
                str(self.config.stop_timeout),
                self._get_image_name(),
                "tail",
                "-f",
                "/dev/null",
            ]
        )

        result = subprocess.run(cmd, env=env)
        return result.returncode == 0

    def transition_to_running(self) -> bool:
        """Ensure container is in running state."""
        try:
            state = self.get_container_state()

            if state == ContainerState.NOT_EXISTS:
                print(f"Creating new container: {self.config.container_name}")
                if not self.create_container():
                    return False
                state = ContainerState.STOPPED

            if state == ContainerState.STOPPED:
                print(f"Starting container: {self.config.container_name}")
                result = subprocess.run(
                    [get_docker_command(), "start", self.config.container_name],
                    timeout=30,
                )
                if result.returncode != 0:
                    return False
            elif state == ContainerState.PAUSED:
                print(f"Unpausing container: {self.config.container_name}")
                result = subprocess.run(
                    [get_docker_command(), "unpause", self.config.container_name],
                    timeout=30,
                )
                if result.returncode != 0:
                    return False
            else:
                print(f"✓ Using existing container: {self.config.container_name}")

            return True
        except FileNotFoundError:
            print("❌ Docker is not available. Please ensure Docker is running.")
            return False
        except subprocess.TimeoutExpired:
            print("❌ Docker command timed out. Docker may not be responding.")
            return False

    def pause(self) -> bool:
        """Pause the container to free resources."""
        try:
            result = subprocess.run(
                [get_docker_command(), "pause", self.config.container_name],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=30,
            )
            return result.returncode == 0
        except (FileNotFoundError, subprocess.TimeoutExpired):
            # Silently fail if Docker is not available
            return False


class DockerEntrypointBuilder:
    """Builds Docker entrypoint scripts with proper syncing logic."""

    @staticmethod
    def build_sync_script() -> str:
        """Generate the rsync script for directory syncing."""
        return """
# Sync host directories to container working directory if they exist
if command -v rsync &> /dev/null; then
    echo "Syncing directories from host..."

    # Define rsync exclude patterns for build artifacts
    RSYNC_EXCLUDES="--exclude=**/__pycache__ --exclude=.build/ --exclude=.pio/ --exclude=*.pyc"

    # Use --checksum to compare file contents instead of timestamps
    # This ensures changes are detected even when timestamps are unreliable across host/container filesystems

    # Start timing the entire sync phase
    SYNC_START=$(date +%s.%N)

    # Create temp files for timing results to avoid race conditions in output
    TMPDIR=$(mktemp -d)

    # Sync project-root pyproject.toml if present
    if [ -f "/host/pyproject.toml" ]; then
        echo "  → syncing pyproject.toml"
        (
            START=$(date +%s.%N)
            # Copy single file to /fastled root. No --delete needed for single-file.
            # --checksum ensures content-based update even with skewed timestamps.
            rsync -a --checksum /host/pyproject.toml /fastled/
            END=$(date +%s.%N)
            echo "$END - $START" | bc > "$TMPDIR/pyproject.time"
        ) &
    fi

    # Run all syncs in parallel, each writing its timing to a temp file
    if [ -d "/host/src" ]; then
        echo "  → syncing src/..."
        (
            START=$(date +%s.%N)
            rsync -a --checksum --delete $RSYNC_EXCLUDES /host/src/ /fastled/src/
            END=$(date +%s.%N)
            echo "$END - $START" | bc > "$TMPDIR/src.time"
        ) &
    fi

    if [ -d "/host/examples" ]; then
        echo "  → syncing examples/..."
        (
            START=$(date +%s.%N)
            rsync -a --checksum --delete $RSYNC_EXCLUDES /host/examples/ /fastled/examples/
            END=$(date +%s.%N)
            echo "$END - $START" | bc > "$TMPDIR/examples.time"
        ) &
    fi

    if [ -d "/host/ci" ]; then
        echo "  → syncing ci/..."
        (
            START=$(date +%s.%N)
            rsync -a --checksum --delete $RSYNC_EXCLUDES /host/ci/ /fastled/ci/
            END=$(date +%s.%N)
            echo "$END - $START" | bc > "$TMPDIR/ci.time"
        ) &
    fi

    # Wait for all parallel syncs to complete
    wait

    # Calculate and display total sync time
    SYNC_END=$(date +%s.%N)
    SYNC_TIME=$(echo "$SYNC_END - $SYNC_START" | bc)

    # Print individual times in deterministic order

    if [ -f "$TMPDIR/pyproject.time" ]; then
        printf "  ✓ pyproject.toml synced in %.3fs\\n" $(cat "$TMPDIR/pyproject.time")
    fi

    if [ -f "$TMPDIR/src.time" ]; then
        printf "  ✓ src/ synced in %.3fs\\n" $(cat "$TMPDIR/src.time")
    fi
    if [ -f "$TMPDIR/examples.time" ]; then
        printf "  ✓ examples/ synced in %.3fs\\n" $(cat "$TMPDIR/examples.time")
    fi
    if [ -f "$TMPDIR/ci.time" ]; then
        printf "  ✓ ci/ synced in %.3fs\\n" $(cat "$TMPDIR/ci.time")
    fi

    printf "  ─────────────────────────────\\n"
    printf "  Total sync time: %.3fs\\n" $SYNC_TIME

    # Cleanup temp files
    rm -rf "$TMPDIR"

    echo ""
    echo "Directory sync complete"

    # Clean Python bytecode cache to ensure latest code is used
    echo "Cleaning Python cache..."
    find /fastled/ci -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true
    find /fastled/ci -type f -name "*.pyc" -delete 2>/dev/null || true
    echo "✓ Python cache cleaned"
else
    echo "Warning: rsync not available, skipping directory sync"
fi
"""

    @staticmethod
    def build_artifact_handling() -> str:
        """Generate artifact handling script."""
        return """
# If /fastled/output is mounted and compilation succeeded, copy build artifacts
if [ -d "/fastled/output" ] && [ $EXIT_CODE -eq 0 ]; then
    echo ""
    echo "Copying build artifacts to output directory..."

    # Find and copy all build artifacts
    if [ -d "/fastled/.pio/build" ]; then
        # Copy all binary files
        find /fastled/.pio/build -type f \\( -name "*.bin" -o -name "*.hex" -o -name "*.elf" -o -name "*.factory.bin" \\) \\
            -exec cp -v {} /fastled/output/ \\; 2>/dev/null || true

        echo "Build artifacts copied to output directory"
    fi
fi
"""

    @staticmethod
    def build_entrypoint(compile_cmd: str, handle_artifacts: bool = True) -> str:
        """Build complete entrypoint script."""
        sync_script = DockerEntrypointBuilder.build_sync_script()
        artifact_handling = (
            DockerEntrypointBuilder.build_artifact_handling()
            if handle_artifacts
            else ""
        )

        return f"""
set -e

{sync_script}

# Execute the compile command
{compile_cmd}
EXIT_CODE=$?

{artifact_handling}

exit $EXIT_CODE
"""


class DockerCompilationOrchestrator:
    """High-level orchestration of Docker compilation workflow."""

    def __init__(self, config: DockerConfig, container_mgr: DockerContainerManager):
        self.config = config
        self.container = container_mgr
        self._actual_image_name: Optional[str] = (
            None  # Tracks the actual image name found
        )

    def _get_image_name(self) -> str:
        """Get the actual image name to use (may be different from config if local)."""
        return self._actual_image_name or self.config.image_name

    def ensure_image_exists(self, build_if_missing: bool = False) -> bool:
        """Ensure Docker image exists, optionally building it.

        Attempts to:
        1. Use existing local image (with or without registry prefix)
        2. Pull from Docker registry (if not building)
        3. Build locally (if build_if_missing=True)
        """
        try:
            result = subprocess.run(
                [get_docker_command(), "image", "inspect", self.config.image_name],
                capture_output=True,
                text=True,
                timeout=10,
            )

            if result.returncode == 0:
                print(f"✓ Using existing Docker image: {self.config.image_name}")
                self._actual_image_name = self.config.image_name
                return True  # Image exists locally

            # Image not found - try without registry prefix (e.g., niteris/fastled-compiler-avr-uno -> fastled-compiler-avr-uno)
            if "/" in self.config.image_name:
                image_without_registry = self.config.image_name.split("/", 1)[1]
                result = subprocess.run(
                    [get_docker_command(), "image", "inspect", image_without_registry],
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                if result.returncode == 0:
                    print(
                        f"✓ Using existing Docker image (local): {image_without_registry}"
                    )
                    self._actual_image_name = image_without_registry
                    return True

            # Image not found locally - try to pull from registry
            print(f"Docker image {self.config.image_name} not found locally.")
            print(f"Attempting to pull from registry...")
            print()

            pull_result = subprocess.run(
                [get_docker_command(), "pull", self.config.image_name],
                capture_output=True,
                text=True,
                timeout=600,  # 10 minute timeout for pull
            )

            if pull_result.returncode == 0:
                print(f"✓ Successfully pulled Docker image: {self.config.image_name}")
                return True

            # Pull failed - show options
            if not build_if_missing:
                print(f"❌ Could not pull Docker image from registry.")
                print(f"   Image: {self.config.image_name}")
                print(f"")
                print(f"Ensure the image exists on Docker Hub:")
                print(f"  https://hub.docker.com/r/niteris")
                print(f"")
                print(f"Or build locally with:")
                print(
                    f"  bash compile --docker --build {self.config.board_name} <example>"
                )
                print(f"")
                return False

            # Build the image locally
            print(f"Building Docker image: {self.config.image_name}")
            print(f"This may take 15-30 minutes depending on the platform...")

            # Parse image name to extract base name (without tag)
            image_parts = self.config.image_name.split(":")
            image_base = image_parts[0]
            image_tag = image_parts[1] if len(image_parts) > 1 else "latest"

            build_cmd = [
                sys.executable,
                "ci/build_docker_image_pio.py",
                "--platform",
                self.config.board_name,
                "--image-name",
                image_base,
                "--tag",
                image_tag,
            ]

            result = subprocess.run(build_cmd)
            return result.returncode == 0
        except (FileNotFoundError, subprocess.TimeoutExpired):
            # Docker command not found or timed out - likely Docker is not running
            return self._handle_docker_not_found()
        except Exception as e:
            print(f"❌ Unexpected error checking Docker image: {e}")
            return False

    def _handle_docker_not_found(self) -> bool:
        """Handle the case where Docker is not installed or running."""
        from ci.util.docker_helper import attempt_start_docker

        print("❌ Docker is not running or not installed")
        print("")
        print("Attempting to start Docker...")
        print("")

        success, message = attempt_start_docker()

        if success:
            print(f"✓ {message}")
            print("")
            return True
        else:
            print(f"ℹ  {message}")
            print("")
            print("Please ensure Docker is installed and running, then try again.")
            print("Docker Desktop: https://www.docker.com/products/docker-desktop")
            return False

    def run_compilation(
        self, compile_cmd: str, stream_output: bool = True
    ) -> "subprocess.CompletedProcess[str]":
        """Execute compilation inside container with proper setup."""
        try:
            if not self.container.transition_to_running():
                raise RuntimeError(
                    f"Failed to start container: {self.config.container_name}"
                )

            print()
            print(f"Running compilation in container: {self.config.container_name}")
            print()

            # Build entrypoint script
            entrypoint = DockerEntrypointBuilder.build_entrypoint(
                compile_cmd, handle_artifacts=True
            )

            # Execute in container
            exec_cmd = [
                get_docker_command(),
                "exec",
                "-e",
                "FASTLED_DOCKER=1",
                self.config.container_name,
                "bash",
                "-c",
                entrypoint,
            ]

            print(f"Executing: {compile_cmd}")
            print()

            if stream_output:
                return self._stream_execution(exec_cmd)
            else:
                return subprocess.run(exec_cmd, capture_output=True, text=True)
        except RuntimeError as e:
            print(f"❌ {e}")
            raise
        except FileNotFoundError:
            print("❌ Docker is not available. Please ensure Docker is running.")
            raise RuntimeError("Docker not available") from None

    def _stream_execution(self, cmd: List[str]) -> "subprocess.CompletedProcess[str]":
        """Execute command with real-time output streaming."""
        sys.stdout.flush()
        sys.stderr.flush()
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            bufsize=1,  # Line buffered
            universal_newlines=True,
            encoding="utf-8",
            errors="replace",  # Replace invalid UTF-8 with replacement character
        )

        if process.stdout:
            for line in process.stdout:
                print(line, end="", flush=True)

        returncode = process.wait()
        return subprocess.CompletedProcess[str](cmd, returncode, stdout="", stderr="")

    def copy_artifacts(self, container_path: str, host_path: Path) -> bool:
        """Copy artifacts from container to host."""
        host_path.mkdir(parents=True, exist_ok=True)

        print()
        print(f"Copying build artifacts from container to host...")

        copy_cmd = [
            get_docker_command(),
            "cp",
            f"{self.config.container_name}:{container_path}/.",
            str(host_path),
        ]

        result = subprocess.run(copy_cmd, capture_output=True, text=True)

        if result.returncode == 0:
            print(f"✅ Build artifacts copied to {host_path}")
            return True
        else:
            print(f"⚠️  Warning: Could not copy artifacts from container")
            if result.stderr:
                print(f"   Error: {result.stderr}")
            return False
