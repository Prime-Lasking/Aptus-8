package main

import (
	"strconv"
	"strings"
)

func strcasecmp(a, b string) bool {
	return strings.EqualFold(a, b)
}

func parseInt(s string) (int64, error) {
	s = strings.TrimSpace(s)
	base := 10
	if strings.HasPrefix(s, "0x") || strings.HasPrefix(s, "0X") {
		base = 16
		s = s[2:]
	}
	return strconv.ParseInt(s, base, 64)
}
