"""Tests for the hello function."""

from pde_software_replication import hello


def test_hello_returns_string():
    """Test that hello() returns a string."""
    result = hello()
    assert isinstance(result, str)


def test_hello_returns_correct_message():
    """Test that hello() returns the expected message."""
    result = hello()
    assert result == "Hello, World!"


def test_hello_is_not_empty():
    """Test that hello() does not return an empty string."""
    result = hello()
    assert len(result) > 0
